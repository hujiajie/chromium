// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_NET_WAKE_ON_WIFI_MANAGER_H_
#define CHROME_BROWSER_CHROMEOS_NET_WAKE_ON_WIFI_MANAGER_H_

#include <memory>

#include "base/containers/scoped_ptr_hash_map.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/power/extension_event_observer.h"
#include "chromeos/network/network_state_handler_observer.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class Profile;

namespace base {
class DictionaryValue;
}

namespace chromeos {

// This class is responsible for managing the various wake-on-wifi related bits
// of functionality in chrome.  It is responsible for communicating the user's
// preferences to shill as well as listening for connections to the Google GCM
// servers and sending that connection information down to shill.  This class is
// owned by ChromeBrowserMainPartsChromeos.  This class is also NOT thread-safe
// and should only be called on the UI thread.
class WakeOnWifiManager : public content::NotificationObserver,
                          public NetworkStateHandlerObserver {
 public:
  enum WakeOnWifiFeature {
    WAKE_ON_WIFI_NONE                    = 0x00,
    WAKE_ON_WIFI_PACKET                  = 0x01,
    WAKE_ON_WIFI_DARKCONNECT             = 0x02,
    WAKE_ON_WIFI_PACKET_AND_DARKCONNECT  = 0x03,
    NOT_SUPPORTED                        = 0x04,
    INVALID                              = 0x08,
  };

  static WakeOnWifiManager* Get();

  WakeOnWifiManager();
  ~WakeOnWifiManager() override;

  // Should be called whenever the primary user changes their preference for the
  // wake-on-wifi features that should be enabled.
  void OnPreferenceChanged(WakeOnWifiFeature feature);

  // Returns true if wake-on-wifi features are supported. Returns false if we
  // have not yet determined whether wake-on-wifi features are supported.
  bool WakeOnWifiSupported();

  // content::NotificationObserver override.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // NetworkStateHandlerObserver overrides.
  void DeviceListChanged() override;
  void DevicePropertiesUpdated(const DeviceState* device) override;

 private:
  // Sends the user's preference to shill, updates the timer used by the GCM
  // client to send heartbeats, and tells |extension_event_observer_| to block
  // (or not block) suspends based on the value of |current_feature_|.
  void HandleWakeOnWifiFeatureUpdated();

  // Requests all the properties for the wifi device from shill.
  void GetWifiDeviceProperties();

  // Callback for getting the Wi-Fi device properties.
  void GetDevicePropertiesCallback(const std::string& device_path,
                                   const base::DictionaryValue& properties);

  // Called when a Profile is added or destroyed.
  void OnProfileAdded(Profile* profile);
  void OnProfileDestroyed(Profile* profile);

  WakeOnWifiFeature current_feature_;

  // Set to true once we have received the properties for the wifi device from
  // shill.
  bool wifi_properties_received_;

  class WakeOnPacketConnectionObserver;
  base::ScopedPtrHashMap<Profile*,
                         std::unique_ptr<WakeOnPacketConnectionObserver>>
      connection_observers_;

  std::unique_ptr<ExtensionEventObserver> extension_event_observer_;

  content::NotificationRegistrar registrar_;

  base::WeakPtrFactory<WakeOnWifiManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(WakeOnWifiManager);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_NET_WAKE_ON_WIFI_MANAGER_H_
