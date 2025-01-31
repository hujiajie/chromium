# Copyright (c) 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Library handling DevTools websocket interaction.
"""

import httplib
import json
import logging
import os
import sys

file_dir = os.path.dirname(__file__)
sys.path.append(os.path.join(file_dir, '..', '..', 'perf'))
from chrome_telemetry_build import chromium_config
sys.path.append(chromium_config.GetTelemetryDir())

from telemetry.internal.backends.chrome_inspector import inspector_websocket
from telemetry.internal.backends.chrome_inspector import websocket

import common_util


DEFAULT_TIMEOUT_SECONDS = 10 # seconds


class DevToolsConnectionException(Exception):
  def __init__(self, message):
    super(DevToolsConnectionException, self).__init__(message)
    logging.warning("DevToolsConnectionException: " + message)


# Taken from telemetry.internal.backends.chrome_inspector.tracing_backend.
# TODO(mattcary): combine this with the above and export?
class _StreamReader(object):
  def __init__(self, inspector, stream_handle):
    self._inspector_websocket = inspector
    self._handle = stream_handle
    self._callback = None
    self._data = None

  def Read(self, callback):
    # Do not allow the instance of this class to be reused, as
    # we only read data sequentially at the moment, so a stream
    # can only be read once.
    assert not self._callback
    self._data = []
    self._callback = callback
    self._ReadChunkFromStream()
    # Queue one extra read ahead to avoid latency.
    self._ReadChunkFromStream()

  def _ReadChunkFromStream(self):
    # Limit max block size to avoid fragmenting memory in sock.recv(),
    # (see https://github.com/liris/websocket-client/issues/163 for details)
    req = {'method': 'IO.read', 'params': {
        'handle': self._handle, 'size': 32768}}
    self._inspector_websocket.AsyncRequest(req, self._GotChunkFromStream)

  def _GotChunkFromStream(self, response):
    # Quietly discard responses from reads queued ahead after EOF.
    if self._data is None:
      return
    if 'error' in response:
      raise DevToolsConnectionException(
          'Reading trace failed: %s' % response['error']['message'])
    result = response['result']
    self._data.append(result['data'])
    if not result.get('eof', False):
      self._ReadChunkFromStream()
      return
    req = {'method': 'IO.close', 'params': {'handle': self._handle}}
    self._inspector_websocket.SendAndIgnoreResponse(req)
    trace_string = ''.join(self._data)
    self._data = None
    self._callback(trace_string)


class DevToolsConnection(object):
  """Handles the communication with a DevTools server.
  """
  TRACING_DOMAIN = 'Tracing'
  TRACING_END_METHOD = 'Tracing.end'
  TRACING_DATA_METHOD = 'Tracing.dataCollected'
  TRACING_DONE_EVENT = 'Tracing.tracingComplete'
  TRACING_STREAM_EVENT = 'Tracing.tracingComplete'  # Same as TRACING_DONE.
  TRACING_TIMEOUT = 300

  def __init__(self, hostname, port):
    """Initializes the connection with a DevTools server.

    Args:
      hostname: server hostname.
      port: port number.
    """
    self._http_hostname = hostname
    self._http_port = port
    self._event_listeners = {}
    self._domain_listeners = {}
    self._scoped_states = {}
    self._domains_to_enable = set()
    self._tearing_down_tracing = False
    self._please_stop = False
    self._ws = None
    self._target_descriptor = None

    self._Connect()

  def RegisterListener(self, name, listener):
    """Registers a listener for an event.

    Also takes care of enabling the relevant domain before starting monitoring.

    Args:
      name: (str) Domain or event the listener wants to listen to, e.g.
            "Network.requestWillBeSent" or "Tracing".
      listener: (Listener) listener instance.
    """
    if '.' in name:
      domain = name[:name.index('.')]
      self._event_listeners[name] = listener
    else:
      domain = name
      self._domain_listeners[domain] = listener
    self._domains_to_enable.add(domain)

  def UnregisterListener(self, listener):
    """Unregisters a listener.

    Args:
      listener: (Listener) listener to unregister.
    """
    keys = ([k for k, l in self._event_listeners if l is listener] +
            [k for k, l in self._domain_listeners if l is listener])
    assert keys, "Removing non-existent listener"
    for key in keys:
      if key in self._event_listeners:
        del(self._event_listeners[key])
      if key in self._domain_listeners:
        del(self._domain_listeners[key])

  def SetScopedState(self, method, params, default_params, enable_domain):
    """Changes state at the beginning the monitoring and resets it at the end.

    |method| is called with |params| at the beginning of the monitoring. After
    the monitoring completes, the state is reset by calling |method| with
    |default_params|.

    Args:
      method: (str) Method.
      params: (dict) Parameters to set when the monitoring starts.
      default_params: (dict) Parameters to reset the state at the end.
      enable_domain: (bool) True if enabling the domain is required.
    """
    if enable_domain:
      if '.' in method:
        domain = method[:method.index('.')]
        assert domain, 'No valid domain'
        self._domains_to_enable.add(domain)
    scoped_state_value = (params, default_params)
    if self._scoped_states.has_key(method):
      assert self._scoped_states[method] == scoped_state_value
    else:
      self._scoped_states[method] = scoped_state_value

  def SyncRequest(self, method, params=None):
    """Issues a synchronous request to the DevTools server.

    Args:
      method: (str) Method.
      params: (dict) Optional parameters to the request.

    Returns:
      The answer.
    """
    request = {'method': method}
    if params:
      request['params'] = params
    return self._ws.SyncRequest(request)

  def SendAndIgnoreResponse(self, method, params=None):
    """Issues a request to the DevTools server, do not wait for the response.

    Args:
      method: (str) Method.
      params: (dict) Optional parameters to the request.
    """
    request = {'method': method}
    if params:
      request['params'] = params
    self._ws.SendAndIgnoreResponse(request)

  def SyncRequestNoResponse(self, method, params=None):
    """As SyncRequest, but asserts that no meaningful response was received.

    Args:
      method: (str) Method.
      params: (dict) Optional parameters to the request.
    """
    result = self.SyncRequest(method, params)
    if 'error' in result or ('result' in result and
                             result['result']):
      raise DevToolsConnectionException(
          'Unexpected response for %s: %s' % (method, result))

  def ClearCache(self):
    """Clears buffer cache.

    Will assert that the browser supports cache clearing.
    """
    res = self.SyncRequest('Network.canClearBrowserCache')
    assert res['result'], 'Cache clearing is not supported by this browser.'
    self.SyncRequest('Network.clearBrowserCache')

  def MonitorUrl(self, url, timeout_seconds=DEFAULT_TIMEOUT_SECONDS):
    """Navigate to url and dispatch monitoring loop.

    Unless you have registered a listener that will call StopMonitoring, this
    will run until timeout from chrome.

    Args:
      url: (str) a URL to navigate to before starting monitoring loop.\
      timeout_seconds: timeout in seconds for monitoring loop.
    """
    for domain in self._domains_to_enable:
      self._ws.RegisterDomain(domain, self._OnDataReceived)
      if domain != self.TRACING_DOMAIN:
        self.SyncRequestNoResponse('%s.enable' % domain)
        # Tracing setup must be done by the tracing track to control filtering
        # and output.
    for scoped_state in self._scoped_states:
      self.SyncRequestNoResponse(scoped_state,
                                 self._scoped_states[scoped_state][0])
    self._tearing_down_tracing = False

    self.SendAndIgnoreResponse('Page.navigate', {'url': url})

    self._Dispatch(timeout=timeout_seconds)
    self._TearDownMonitoring()

  def StopMonitoring(self):
    """Stops the monitoring."""
    self._please_stop = True

  def ExecuteJavaScript(self, expression):
    """Run JavaScript expression.

    Args:
      expression: JavaScript expression to run.

    Returns:
      The return value from the JavaScript expression.
    """
    response = self.SyncRequest('Runtime.evaluate', {
        'expression': expression,
        'returnByValue': True})
    if 'error' in response:
      raise Exception(response['error']['message'])
    if 'wasThrown' in response['result'] and response['result']['wasThrown']:
      raise Exception(response['error']['result']['description'])
    if response['result']['result']['type'] == 'undefined':
      return None
    return response['result']['result']['value']

  def PollForJavaScriptExpression(self, expression, interval):
    """Wait until JavaScript expression is true.

    Args:
      expression: JavaScript expression to run.
      interval: Period between expression evaluation in seconds.
    """
    common_util.PollFor(lambda: bool(self.ExecuteJavaScript(expression)),
                        'JavaScript: {}'.format(expression),
                        interval)

  def Close(self):
    """Cleanly close chrome by closing the only tab."""
    assert self._ws
    response = self._HttpRequest('/close/' + self._target_descriptor['id'])
    assert response == 'Target is closing'
    self._ws = None

  def _Dispatch(self, timeout, kind='Monitoring'):
    self._please_stop = False
    while not self._please_stop:
      try:
        self._ws.DispatchNotifications(timeout=timeout)
      except websocket.WebSocketTimeoutException:
        break
    if not self._please_stop:
      logging.warning('%s stopped on a timeout.' % kind)

  def _TearDownMonitoring(self):
    if self.TRACING_DOMAIN in self._domains_to_enable:
      logging.info('Fetching tracing')
      self.SyncRequestNoResponse(self.TRACING_END_METHOD)
      self._tearing_down_tracing = True
      self._Dispatch(timeout=self.TRACING_TIMEOUT, kind='Tracing')
    for scoped_state in self._scoped_states:
      self.SyncRequestNoResponse(scoped_state,
                                 self._scoped_states[scoped_state][1])
    for domain in self._domains_to_enable:
      if domain != self.TRACING_DOMAIN:
        self.SyncRequest('%s.disable' % domain)
      self._ws.UnregisterDomain(domain)
    self._domains_to_enable.clear()
    self._domain_listeners.clear()
    self._event_listeners.clear()
    self._scoped_states.clear()

  def _OnDataReceived(self, msg):
    if 'method' not in msg:
      raise DevToolsConnectionException('Malformed message: %s' % msg)
    method = msg['method']
    domain = method[:method.index('.')]

    if self._tearing_down_tracing and method == self.TRACING_STREAM_EVENT:
      stream_handle = msg.get('params', {}).get('stream')
      if not stream_handle:
        self._tearing_down_tracing = False
        self.StopMonitoring()
        # Fall through to regular dispatching.
      else:
        _StreamReader(self._ws, stream_handle).Read(self._TracingStreamDone)
        # Skip regular dispatching.
        return

    if (method not in self._event_listeners and
        domain not in self._domain_listeners):
      return
    if method in self._event_listeners:
      self._event_listeners[method].Handle(method, msg)
    if domain in self._domain_listeners:
      self._domain_listeners[domain].Handle(method, msg)
    if self._tearing_down_tracing and method == self.TRACING_DONE_EVENT:
      self._tearing_down_tracing = False
      self.StopMonitoring()

  def _TracingStreamDone(self, data):
    tracing_events = json.loads(data)
    for evt in tracing_events:
      self._OnDataReceived({'method': self.TRACING_DATA_METHOD,
                            'params': {'value': [evt]}})
      if self._please_stop:
        break
    self._tearing_down_tracing = False
    self.StopMonitoring()

  def _HttpRequest(self, path):
    assert path[0] == '/'
    r = httplib.HTTPConnection(self._http_hostname, self._http_port)
    try:
      r.request('GET', '/json' + path)
      response = r.getresponse()
      if response.status != 200:
        raise DevToolsConnectionException(
            'Cannot connect to DevTools, reponse code %d' % response.status)
      raw_response = response.read()
    finally:
      r.close()
    return raw_response

  def _Connect(self):
    assert not self._ws
    assert not self._target_descriptor
    for target_descriptor in json.loads(self._HttpRequest('/list')):
      if target_descriptor['type'] == 'page':
        self._target_descriptor = target_descriptor
        break
    assert self._target_descriptor['url'] == 'about:blank'
    self._ws = inspector_websocket.InspectorWebsocket()
    self._ws.Connect(self._target_descriptor['webSocketDebuggerUrl'])


class Listener(object):
  """Listens to events forwarded by a DevToolsConnection instance."""
  def __init__(self, connection):
    """Initializes a Listener instance.

    Args:
      connection: (DevToolsConnection).
    """
    pass

  def Handle(self, method, msg):
    """Handles an event this instance listens for.

    Args:
      event_name: (str) Event name, as registered.
      event: (dict) complete event.
    """
    pass


class Track(Listener):
  """Collects data from a DevTools server."""
  def GetEvents(self):
    """Returns a list of collected events, finalizing the state if necessary."""
    pass

  def ToJsonDict(self):
    """Serializes to a dictionary, to be dumped as JSON.

    Returns:
      A dict that can be dumped by the json module, and loaded by
      FromJsonDict().
    """
    pass

  @classmethod
  def FromJsonDict(cls, json_dict):
    """Returns a Track instance constructed from data dumped by
       Track.ToJsonDict().

    Args:
      json_data: (dict) Parsed from a JSON file using the json module.

    Returns:
      a Track instance.
    """
    pass
