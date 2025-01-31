// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "printing/pdf_transform.h"

#include "printing/units.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/rect.h"

namespace printing {

namespace {

const float kDefaultWidth = 8.5 * kPointsPerInch;
const float kDefaultHeight = 11.0 * kPointsPerInch;
const float kDefaultRatio = kDefaultWidth / kDefaultHeight;
const double kTolerance = 0.0001;

void ExpectDefaultPortraitBox(const ClipBox& box) {
  EXPECT_FLOAT_EQ(0, box.left);
  EXPECT_FLOAT_EQ(kDefaultWidth, box.right);
  EXPECT_FLOAT_EQ(kDefaultHeight, box.top);
  EXPECT_FLOAT_EQ(0, box.bottom);
}

void ExpectDefaultLandscapeBox(const ClipBox& box) {
  EXPECT_FLOAT_EQ(0, box.left);
  EXPECT_FLOAT_EQ(kDefaultHeight, box.right);
  EXPECT_FLOAT_EQ(kDefaultWidth, box.top);
  EXPECT_FLOAT_EQ(0, box.bottom);
}

void ExpectBoxesAreEqual(const ClipBox& expected, const ClipBox& actual) {
  EXPECT_FLOAT_EQ(expected.left, actual.left);
  EXPECT_FLOAT_EQ(expected.right, actual.right);
  EXPECT_FLOAT_EQ(expected.top, actual.top);
  EXPECT_FLOAT_EQ(expected.bottom, actual.bottom);
}

void InitializeBoxToInvalidValues(ClipBox* box) {
  box->left = box->right = box->top = box->bottom = -1;
}

void InitializeBoxToDefaultPortraitValues(ClipBox* box) {
  box->left = 0;
  box->right = kDefaultWidth;
  box->top = kDefaultHeight;
  box->bottom = 0;
}

void InitializeBoxToDefaultLandscapeValue(ClipBox* box) {
  box->left = 0;
  box->right = kDefaultHeight;
  box->top = kDefaultWidth;
  box->bottom = 0;
}

}  // namespace

TEST(PdfTransformTest, CalculateScaleFactor) {
  gfx::Rect rect(kDefaultWidth, kDefaultHeight);
  double scale;

  // 1:1
  scale = CalculateScaleFactor(rect, kDefaultWidth, kDefaultHeight, false);
  EXPECT_NEAR(1, scale, kTolerance);
  scale = CalculateScaleFactor(rect, kDefaultWidth, kDefaultHeight, true);
  EXPECT_NEAR(kDefaultRatio, scale, kTolerance);

  // 1:2
  rect = gfx::Rect(kDefaultWidth / 2, kDefaultHeight / 2);
  scale = CalculateScaleFactor(rect, kDefaultWidth, kDefaultHeight, false);
  EXPECT_NEAR(0.5, scale, kTolerance);
  scale = CalculateScaleFactor(rect, kDefaultWidth, kDefaultHeight, true);
  EXPECT_NEAR(kDefaultRatio / 2, scale, kTolerance);

  // 3:1
  rect = gfx::Rect(kDefaultWidth * 3, kDefaultHeight * 3);
  scale = CalculateScaleFactor(rect, kDefaultWidth, kDefaultHeight, false);
  EXPECT_NEAR(3, scale, kTolerance);
  scale = CalculateScaleFactor(rect, kDefaultWidth, kDefaultHeight, true);
  EXPECT_NEAR(kDefaultRatio * 3, scale, kTolerance);

  // 3:1, rotated.
  rect = gfx::Rect(kDefaultHeight * 3, kDefaultWidth * 3);
  scale = CalculateScaleFactor(rect, kDefaultWidth, kDefaultHeight, false);
  EXPECT_NEAR(kDefaultRatio * 3, scale, kTolerance);
  scale = CalculateScaleFactor(rect, kDefaultWidth, kDefaultHeight, true);
  EXPECT_NEAR(3, scale, kTolerance);

  // Odd size
  rect = gfx::Rect(10, 1000);
  scale = CalculateScaleFactor(rect, kDefaultWidth, kDefaultHeight, false);
  EXPECT_NEAR(0.01634, scale, kTolerance);
  scale = CalculateScaleFactor(rect, kDefaultWidth, kDefaultHeight, true);
  EXPECT_NEAR(0.01263, scale, kTolerance);
}

TEST(PdfTransformTest, SetDefaultClipBox) {
  ClipBox box;

  SetDefaultClipBox(false, &box);
  ExpectDefaultPortraitBox(box);

  SetDefaultClipBox(true, &box);
  ExpectDefaultLandscapeBox(box);
}

TEST(PdfTransformTest, CalculateMediaBoxAndCropBox) {
  ClipBox media_box;
  ClipBox crop_box;

  // Assume both boxes are there.
  InitializeBoxToDefaultPortraitValues(&media_box);
  InitializeBoxToDefaultLandscapeValue(&crop_box);
  CalculateMediaBoxAndCropBox(true, true, true, &media_box, &crop_box);
  ExpectDefaultPortraitBox(media_box);
  ExpectDefaultLandscapeBox(crop_box);

  // Assume both boxes are missing.
  InitializeBoxToInvalidValues(&media_box);
  InitializeBoxToInvalidValues(&crop_box);
  CalculateMediaBoxAndCropBox(false, false, false, &media_box, &crop_box);
  ExpectDefaultPortraitBox(media_box);
  ExpectDefaultPortraitBox(crop_box);
  CalculateMediaBoxAndCropBox(true, false, false, &media_box, &crop_box);
  ExpectDefaultLandscapeBox(media_box);
  ExpectDefaultLandscapeBox(crop_box);

  // Assume crop box is missing.
  ClipBox expected_box;
  expected_box.left = 0;
  expected_box.right = 42;
  expected_box.top = 420;
  expected_box.bottom = 0;
  media_box = expected_box;
  InitializeBoxToInvalidValues(&crop_box);
  CalculateMediaBoxAndCropBox(false, true, false, &media_box, &crop_box);
  ExpectBoxesAreEqual(expected_box, media_box);
  ExpectBoxesAreEqual(expected_box, crop_box);

  // Assume media box is missing.
  InitializeBoxToInvalidValues(&media_box);
  CalculateMediaBoxAndCropBox(false, false, true, &media_box, &crop_box);
  ExpectBoxesAreEqual(expected_box, media_box);
  ExpectBoxesAreEqual(expected_box, crop_box);
}

TEST(PdfTransformTest, CalculateClipBoxBoundary) {
  ClipBox media_box;
  ClipBox crop_box;
  ClipBox result;

  // media box and crop box are the same.
  InitializeBoxToDefaultPortraitValues(&media_box);
  InitializeBoxToDefaultPortraitValues(&crop_box);
  result = CalculateClipBoxBoundary(media_box, crop_box);
  ExpectDefaultPortraitBox(result);

  // media box is portrait and crop box is landscape.
  InitializeBoxToDefaultLandscapeValue(&crop_box);
  result = CalculateClipBoxBoundary(media_box, crop_box);
  EXPECT_FLOAT_EQ(0, result.left);
  EXPECT_FLOAT_EQ(kDefaultWidth, result.right);
  EXPECT_FLOAT_EQ(kDefaultWidth, result.top);
  EXPECT_FLOAT_EQ(0, result.bottom);

  // crop box is smaller than media box.
  crop_box.left = 0;
  crop_box.right = 100;
  crop_box.bottom = 0;
  crop_box.top = 200;
  result = CalculateClipBoxBoundary(media_box, crop_box);
  EXPECT_FLOAT_EQ(0, result.left);
  EXPECT_FLOAT_EQ(100, result.right);
  EXPECT_FLOAT_EQ(200, result.top);
  EXPECT_FLOAT_EQ(0, result.bottom);

  // crop box is smaller than the media box in one dimension and longer in the
  // other.
  crop_box.left = 0;
  crop_box.right = 100;
  crop_box.bottom = 0;
  crop_box.top = 2000;
  result = CalculateClipBoxBoundary(media_box, crop_box);
  EXPECT_FLOAT_EQ(0, result.left);
  EXPECT_FLOAT_EQ(100, result.right);
  EXPECT_FLOAT_EQ(kDefaultHeight, result.top);
  EXPECT_FLOAT_EQ(0, result.bottom);
}

TEST(PdfTransformTest, CalculateScaledClipBoxOffset) {
  const gfx::Rect rect(kDefaultWidth, kDefaultHeight);
  ClipBox clip_box;
  double offset_x;
  double offset_y;

  // |rect| and |clip_box| are the same size.
  InitializeBoxToDefaultPortraitValues(&clip_box);
  CalculateScaledClipBoxOffset(rect, clip_box, &offset_x, &offset_y);
  EXPECT_DOUBLE_EQ(0, offset_x);
  EXPECT_DOUBLE_EQ(0, offset_y);

  // |rect| is larger than |clip_box|.
  clip_box.top /= 2;
  clip_box.right /= 4;
  CalculateScaledClipBoxOffset(rect, clip_box, &offset_x, &offset_y);
  EXPECT_DOUBLE_EQ(229.5, offset_x);
  EXPECT_DOUBLE_EQ(198, offset_y);
}

TEST(PdfTransformTest, CalculateNonScaledClipBoxOffset) {
  int page_width = kDefaultWidth;
  int page_height = kDefaultHeight;
  const gfx::Rect rect(kDefaultWidth, kDefaultHeight);
  ClipBox clip_box;
  double offset_x;
  double offset_y;

  // |rect|, page size and |clip_box| are the same.
  InitializeBoxToDefaultPortraitValues(&clip_box);
  CalculateNonScaledClipBoxOffset(
      rect, 0, page_width, page_height, clip_box, &offset_x, &offset_y);
  EXPECT_DOUBLE_EQ(0, offset_x);
  EXPECT_DOUBLE_EQ(0, offset_y);
  CalculateNonScaledClipBoxOffset(
      rect, 1, page_width, page_height, clip_box, &offset_x, &offset_y);
  EXPECT_DOUBLE_EQ(0, offset_x);
  EXPECT_DOUBLE_EQ(0, offset_y);
  CalculateNonScaledClipBoxOffset(
      rect, 2, page_width, page_height, clip_box, &offset_x, &offset_y);
  EXPECT_DOUBLE_EQ(0, offset_x);
  EXPECT_DOUBLE_EQ(0, offset_y);
  CalculateNonScaledClipBoxOffset(
      rect, 3, page_width, page_height, clip_box, &offset_x, &offset_y);
  EXPECT_DOUBLE_EQ(180, offset_x);
  EXPECT_DOUBLE_EQ(-180, offset_y);

  // Smaller |clip_box|.
  clip_box.top /= 4;
  clip_box.right /= 2;
  CalculateNonScaledClipBoxOffset(
      rect, 0, page_width, page_height, clip_box, &offset_x, &offset_y);
  EXPECT_DOUBLE_EQ(0, offset_x);
  EXPECT_DOUBLE_EQ(594, offset_y);
  CalculateNonScaledClipBoxOffset(
      rect, 1, page_width, page_height, clip_box, &offset_x, &offset_y);
  EXPECT_DOUBLE_EQ(0, offset_x);
  EXPECT_DOUBLE_EQ(0, offset_y);
  CalculateNonScaledClipBoxOffset(
      rect, 2, page_width, page_height, clip_box, &offset_x, &offset_y);
  EXPECT_DOUBLE_EQ(306, offset_x);
  EXPECT_DOUBLE_EQ(0, offset_y);
  CalculateNonScaledClipBoxOffset(
      rect, 3, page_width, page_height, clip_box, &offset_x, &offset_y);
  EXPECT_DOUBLE_EQ(486, offset_x);
  EXPECT_DOUBLE_EQ(414, offset_y);

  // Larger page size.
  InitializeBoxToDefaultPortraitValues(&clip_box);
  page_width += 10;
  page_height += 20;
  CalculateNonScaledClipBoxOffset(
      rect, 0, page_width, page_height, clip_box, &offset_x, &offset_y);
  EXPECT_DOUBLE_EQ(0, offset_x);
  EXPECT_DOUBLE_EQ(20, offset_y);
  CalculateNonScaledClipBoxOffset(
      rect, 1, page_width, page_height, clip_box, &offset_x, &offset_y);
  EXPECT_DOUBLE_EQ(0, offset_x);
  EXPECT_DOUBLE_EQ(0, offset_y);
  CalculateNonScaledClipBoxOffset(
      rect, 2, page_width, page_height, clip_box, &offset_x, &offset_y);
  EXPECT_DOUBLE_EQ(10, offset_x);
  EXPECT_DOUBLE_EQ(0, offset_y);
  CalculateNonScaledClipBoxOffset(
      rect, 3, page_width, page_height, clip_box, &offset_x, &offset_y);
  EXPECT_DOUBLE_EQ(200, offset_x);
  EXPECT_DOUBLE_EQ(-170, offset_y);
}

// https://crbug.com/491160 and https://crbug.com/588757
TEST(PdfTransformTest, ReversedMediaBox) {
  int page_width = kDefaultWidth;
  int page_height = kDefaultHeight;
  const gfx::Rect rect(kDefaultWidth, kDefaultHeight);
  ClipBox clip_box;
  double offset_x;
  double offset_y;

  const ClipBox expected_media_box_b491160 = {0, 612, 0, -792};
  ClipBox media_box_b491160 = {0, 612, -792, 0};
  CalculateMediaBoxAndCropBox(false, true, false, &media_box_b491160,
                              &clip_box);
  ExpectBoxesAreEqual(expected_media_box_b491160, media_box_b491160);
  ExpectBoxesAreEqual(expected_media_box_b491160, clip_box);

  CalculateScaledClipBoxOffset(rect, media_box_b491160, &offset_x, &offset_y);
  EXPECT_DOUBLE_EQ(0, offset_x);
  EXPECT_DOUBLE_EQ(792, offset_y);

  CalculateNonScaledClipBoxOffset(rect, 0, page_width, page_height,
                                  media_box_b491160, &offset_x, &offset_y);
  EXPECT_DOUBLE_EQ(0, offset_x);
  EXPECT_DOUBLE_EQ(792, offset_y);

  ClipBox media_box_b588757 = {0, 612, 0, 792};
  CalculateMediaBoxAndCropBox(false, true, false, &media_box_b588757,
                              &clip_box);
  ExpectDefaultPortraitBox(media_box_b588757);
  ExpectDefaultPortraitBox(clip_box);

  CalculateScaledClipBoxOffset(rect, clip_box, &offset_x, &offset_y);
  EXPECT_DOUBLE_EQ(0, offset_x);
  EXPECT_DOUBLE_EQ(0, offset_y);

  CalculateNonScaledClipBoxOffset(rect, 0, page_width, page_height, clip_box,
                                  &offset_x, &offset_y);
  EXPECT_DOUBLE_EQ(0, offset_x);
  EXPECT_DOUBLE_EQ(0, offset_y);

  ClipBox media_box_left_right_flipped = {612, 0, 0, 792};
  CalculateMediaBoxAndCropBox(false, true, false, &media_box_left_right_flipped,
                              &clip_box);
  ExpectDefaultPortraitBox(media_box_left_right_flipped);
  ExpectDefaultPortraitBox(clip_box);

  CalculateScaledClipBoxOffset(rect, clip_box, &offset_x, &offset_y);
  EXPECT_DOUBLE_EQ(0, offset_x);
  EXPECT_DOUBLE_EQ(0, offset_y);

  CalculateNonScaledClipBoxOffset(rect, 0, page_width, page_height, clip_box,
                                  &offset_x, &offset_y);
  EXPECT_DOUBLE_EQ(0, offset_x);
  EXPECT_DOUBLE_EQ(0, offset_y);
}

}  // namespace printing
