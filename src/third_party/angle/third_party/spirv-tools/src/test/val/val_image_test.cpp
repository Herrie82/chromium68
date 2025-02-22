// Copyright (c) 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Tests for unique type declaration rules validator.

#include <sstream>
#include <string>

#include "gmock/gmock.h"
#include "unit_spirv.h"
#include "val_fixtures.h"

namespace {

using ::testing::HasSubstr;
using ::testing::Not;

using ValidateImage = spvtest::ValidateBase<bool>;

std::string GenerateShaderCode(
    const std::string& body,
    const std::string& capabilities_and_extensions = "",
    const std::string& execution_model = "Fragment") {
  std::ostringstream ss;
  ss << R"(
OpCapability Float16
OpCapability Shader
OpCapability InputAttachment
OpCapability ImageGatherExtended
OpCapability MinLod
OpCapability Sampled1D
OpCapability SampledRect
OpCapability ImageQuery
)";

  ss << capabilities_and_extensions;
  ss << "OpMemoryModel Logical GLSL450\n";
  ss << "OpEntryPoint " << execution_model << " %main \"main\"\n";

  ss << R"(
%void = OpTypeVoid
%func = OpTypeFunction %void
%bool = OpTypeBool
%f16 = OpTypeFloat 16
%f32 = OpTypeFloat 32
%u32 = OpTypeInt 32 0
%s32 = OpTypeInt 32 1
%s32vec2 = OpTypeVector %s32 2
%u32vec2 = OpTypeVector %u32 2
%f32vec2 = OpTypeVector %f32 2
%u32vec3 = OpTypeVector %u32 3
%s32vec3 = OpTypeVector %s32 3
%f32vec3 = OpTypeVector %f32 3
%u32vec4 = OpTypeVector %u32 4
%s32vec4 = OpTypeVector %s32 4
%f32vec4 = OpTypeVector %f32 4

%f16_0 = OpConstant %f16 0
%f16_1 = OpConstant %f16 1

%f32_0 = OpConstant %f32 0
%f32_1 = OpConstant %f32 1
%f32_0_5 = OpConstant %f32 0.5
%f32_0_25 = OpConstant %f32 0.25
%f32_0_75 = OpConstant %f32 0.75

%s32_0 = OpConstant %s32 0
%s32_1 = OpConstant %s32 1
%s32_2 = OpConstant %s32 2
%s32_3 = OpConstant %s32 3
%s32_4 = OpConstant %s32 4
%s32_m1 = OpConstant %s32 -1

%u32_0 = OpConstant %u32 0
%u32_1 = OpConstant %u32 1
%u32_2 = OpConstant %u32 2
%u32_3 = OpConstant %u32 3
%u32_4 = OpConstant %u32 4

%u32vec2arr4 = OpTypeArray %u32vec2 %u32_4
%u32vec2arr3 = OpTypeArray %u32vec2 %u32_3
%u32arr4 = OpTypeArray %u32 %u32_4
%u32vec3arr4 = OpTypeArray %u32vec3 %u32_4

%u32vec2_01 = OpConstantComposite %u32vec2 %u32_0 %u32_1
%u32vec2_12 = OpConstantComposite %u32vec2 %u32_1 %u32_2
%u32vec3_012 = OpConstantComposite %u32vec3 %u32_0 %u32_1 %u32_2
%u32vec3_123 = OpConstantComposite %u32vec3 %u32_1 %u32_2 %u32_3
%u32vec4_0123 = OpConstantComposite %u32vec4 %u32_0 %u32_1 %u32_2 %u32_3
%u32vec4_1234 = OpConstantComposite %u32vec4 %u32_1 %u32_2 %u32_3 %u32_4

%s32vec2_01 = OpConstantComposite %s32vec2 %s32_0 %s32_1
%s32vec2_12 = OpConstantComposite %s32vec2 %s32_1 %s32_2
%s32vec3_012 = OpConstantComposite %s32vec3 %s32_0 %s32_1 %s32_2
%s32vec3_123 = OpConstantComposite %s32vec3 %s32_1 %s32_2 %s32_3
%s32vec4_0123 = OpConstantComposite %s32vec4 %s32_0 %s32_1 %s32_2 %s32_3
%s32vec4_1234 = OpConstantComposite %s32vec4 %s32_1 %s32_2 %s32_3 %s32_4

%f32vec2_00 = OpConstantComposite %f32vec2 %f32_0 %f32_0
%f32vec2_01 = OpConstantComposite %f32vec2 %f32_0 %f32_1
%f32vec2_10 = OpConstantComposite %f32vec2 %f32_1 %f32_0
%f32vec2_11 = OpConstantComposite %f32vec2 %f32_1 %f32_1
%f32vec2_hh = OpConstantComposite %f32vec2 %f32_0_5 %f32_0_5

%f32vec3_000 = OpConstantComposite %f32vec3 %f32_0 %f32_0 %f32_0
%f32vec3_hhh = OpConstantComposite %f32vec3 %f32_0_5 %f32_0_5 %f32_0_5

%f32vec4_0000 = OpConstantComposite %f32vec4 %f32_0 %f32_0 %f32_0 %f32_0

%const_offsets = OpConstantComposite %u32vec2arr4 %u32vec2_01 %u32vec2_12 %u32vec2_01 %u32vec2_12
%const_offsets3x2 = OpConstantComposite %u32vec2arr3 %u32vec2_01 %u32vec2_12 %u32vec2_01
%const_offsets4xu = OpConstantComposite %u32arr4 %u32_0 %u32_0 %u32_0 %u32_0
%const_offsets4x3 = OpConstantComposite %u32vec3arr4 %u32vec3_012 %u32vec3_012 %u32vec3_012 %u32vec3_012

%type_image_f32_1d_0001 = OpTypeImage %f32 1D 0 0 0 1 Unknown
%ptr_image_f32_1d_0001 = OpTypePointer UniformConstant %type_image_f32_1d_0001
%uniform_image_f32_1d_0001 = OpVariable %ptr_image_f32_1d_0001 UniformConstant
%type_sampled_image_f32_1d_0001 = OpTypeSampledImage %type_image_f32_1d_0001

%type_image_f32_1d_0002_rgba32f = OpTypeImage %f32 1D 0 0 0 2 Rgba32f
%ptr_image_f32_1d_0002_rgba32f = OpTypePointer UniformConstant %type_image_f32_1d_0002_rgba32f
%uniform_image_f32_1d_0002_rgba32f = OpVariable %ptr_image_f32_1d_0002_rgba32f UniformConstant
%type_sampled_image_f32_1d_0002_rgba32f = OpTypeSampledImage %type_image_f32_1d_0002_rgba32f

%type_image_f32_2d_0001 = OpTypeImage %f32 2D 0 0 0 1 Unknown
%ptr_image_f32_2d_0001 = OpTypePointer UniformConstant %type_image_f32_2d_0001
%uniform_image_f32_2d_0001 = OpVariable %ptr_image_f32_2d_0001 UniformConstant
%type_sampled_image_f32_2d_0001 = OpTypeSampledImage %type_image_f32_2d_0001

%type_image_f32_2d_0010 = OpTypeImage %f32 2D 0 0 1 0 Unknown
%ptr_image_f32_2d_0010 = OpTypePointer UniformConstant %type_image_f32_2d_0010
%uniform_image_f32_2d_0010 = OpVariable %ptr_image_f32_2d_0010 UniformConstant
%type_sampled_image_f32_2d_0010 = OpTypeSampledImage %type_image_f32_2d_0010

%type_image_u32_2d_0001 = OpTypeImage %u32 2D 0 0 0 1 Unknown
%ptr_image_u32_2d_0001 = OpTypePointer UniformConstant %type_image_u32_2d_0001
%uniform_image_u32_2d_0001 = OpVariable %ptr_image_u32_2d_0001 UniformConstant
%type_sampled_image_u32_2d_0001 = OpTypeSampledImage %type_image_u32_2d_0001

%type_image_u32_2d_0000 = OpTypeImage %u32 2D 0 0 0 0 Unknown
%ptr_image_u32_2d_0000 = OpTypePointer UniformConstant %type_image_u32_2d_0000
%uniform_image_u32_2d_0000 = OpVariable %ptr_image_u32_2d_0000 UniformConstant
%type_sampled_image_u32_2d_0000 = OpTypeSampledImage %type_image_u32_2d_0000

%type_image_s32_3d_0001 = OpTypeImage %s32 3D 0 0 0 1 Unknown
%ptr_image_s32_3d_0001 = OpTypePointer UniformConstant %type_image_s32_3d_0001
%uniform_image_s32_3d_0001 = OpVariable %ptr_image_s32_3d_0001 UniformConstant
%type_sampled_image_s32_3d_0001 = OpTypeSampledImage %type_image_s32_3d_0001

%type_image_void_2d_0001 = OpTypeImage %void 2D 0 0 0 1 Unknown
%ptr_image_void_2d_0001 = OpTypePointer UniformConstant %type_image_void_2d_0001
%uniform_image_void_2d_0001 = OpVariable %ptr_image_void_2d_0001 UniformConstant
%type_sampled_image_void_2d_0001 = OpTypeSampledImage %type_image_void_2d_0001

%type_image_void_2d_0002 = OpTypeImage %void 2D 0 0 0 2 Unknown
%ptr_image_void_2d_0002 = OpTypePointer UniformConstant %type_image_void_2d_0002
%uniform_image_void_2d_0002 = OpVariable %ptr_image_void_2d_0002 UniformConstant
%type_sampled_image_void_2d_0002 = OpTypeSampledImage %type_image_void_2d_0002

%type_image_f32_2d_0002 = OpTypeImage %f32 2D 0 0 0 2 Unknown
%ptr_image_f32_2d_0002 = OpTypePointer UniformConstant %type_image_f32_2d_0002
%uniform_image_f32_2d_0002 = OpVariable %ptr_image_f32_2d_0001 UniformConstant
%type_sampled_image_f32_2d_0002 = OpTypeSampledImage %type_image_f32_2d_0002

%type_image_f32_spd_0002 = OpTypeImage %f32 SubpassData 0 0 0 2 Unknown
%ptr_image_f32_spd_0002 = OpTypePointer UniformConstant %type_image_f32_spd_0002
%uniform_image_f32_spd_0002 = OpVariable %ptr_image_f32_spd_0002 UniformConstant
%type_sampled_image_f32_spd_0002 = OpTypeSampledImage %type_image_f32_spd_0002

%type_image_f32_3d_0111 = OpTypeImage %f32 3D 0 1 1 1 Unknown
%ptr_image_f32_3d_0111 = OpTypePointer UniformConstant %type_image_f32_3d_0111
%uniform_image_f32_3d_0111 = OpVariable %ptr_image_f32_3d_0111 UniformConstant
%type_sampled_image_f32_3d_0111 = OpTypeSampledImage %type_image_f32_3d_0111

%type_image_f32_cube_0101 = OpTypeImage %f32 Cube 0 1 0 1 Unknown
%ptr_image_f32_cube_0101 = OpTypePointer UniformConstant %type_image_f32_cube_0101
%uniform_image_f32_cube_0101 = OpVariable %ptr_image_f32_cube_0101 UniformConstant
%type_sampled_image_f32_cube_0101 = OpTypeSampledImage %type_image_f32_cube_0101

%type_image_f32_cube_0102_rgba32f = OpTypeImage %f32 Cube 0 1 0 2 Rgba32f
%ptr_image_f32_cube_0102_rgba32f = OpTypePointer UniformConstant %type_image_f32_cube_0102_rgba32f
%uniform_image_f32_cube_0102_rgba32f = OpVariable %ptr_image_f32_cube_0102_rgba32f UniformConstant
%type_sampled_image_f32_cube_0102_rgba32f = OpTypeSampledImage %type_image_f32_cube_0102_rgba32f

%type_image_f32_rect_0001 = OpTypeImage %f32 Rect 0 0 0 1 Unknown
%ptr_image_f32_rect_0001 = OpTypePointer UniformConstant %type_image_f32_rect_0001
%uniform_image_f32_rect_0001 = OpVariable %ptr_image_f32_rect_0001 UniformConstant
%type_sampled_image_f32_rect_0001 = OpTypeSampledImage %type_image_f32_rect_0001

%type_sampler = OpTypeSampler
%ptr_sampler = OpTypePointer UniformConstant %type_sampler
%uniform_sampler = OpVariable %ptr_sampler UniformConstant

%main = OpFunction %void None %func
%main_entry = OpLabel
)";

  ss << body;

  ss << R"(
OpReturn
OpFunctionEnd)";

  return ss.str();
}

std::string GenerateKernelCode(
    const std::string& body,
    const std::string& capabilities_and_extensions = "") {
  std::ostringstream ss;
  ss << R"(
OpCapability Addresses
OpCapability Kernel
OpCapability Linkage
OpCapability ImageQuery
OpCapability ImageGatherExtended
OpCapability InputAttachment
OpCapability SampledRect
)";

  ss << capabilities_and_extensions;
  ss << R"(
OpMemoryModel Physical32 OpenCL
%void = OpTypeVoid
%func = OpTypeFunction %void
%bool = OpTypeBool
%f32 = OpTypeFloat 32
%u32 = OpTypeInt 32 0
%u32vec2 = OpTypeVector %u32 2
%f32vec2 = OpTypeVector %f32 2
%u32vec3 = OpTypeVector %u32 3
%f32vec3 = OpTypeVector %f32 3
%u32vec4 = OpTypeVector %u32 4
%f32vec4 = OpTypeVector %f32 4

%f32_0 = OpConstant %f32 0
%f32_1 = OpConstant %f32 1
%f32_0_5 = OpConstant %f32 0.5
%f32_0_25 = OpConstant %f32 0.25
%f32_0_75 = OpConstant %f32 0.75

%u32_0 = OpConstant %u32 0
%u32_1 = OpConstant %u32 1
%u32_2 = OpConstant %u32 2
%u32_3 = OpConstant %u32 3
%u32_4 = OpConstant %u32 4

%u32vec2_01 = OpConstantComposite %u32vec2 %u32_0 %u32_1
%u32vec2_12 = OpConstantComposite %u32vec2 %u32_1 %u32_2
%u32vec3_012 = OpConstantComposite %u32vec3 %u32_0 %u32_1 %u32_2
%u32vec3_123 = OpConstantComposite %u32vec3 %u32_1 %u32_2 %u32_3
%u32vec4_0123 = OpConstantComposite %u32vec4 %u32_0 %u32_1 %u32_2 %u32_3
%u32vec4_1234 = OpConstantComposite %u32vec4 %u32_1 %u32_2 %u32_3 %u32_4

%f32vec2_00 = OpConstantComposite %f32vec2 %f32_0 %f32_0
%f32vec2_01 = OpConstantComposite %f32vec2 %f32_0 %f32_1
%f32vec2_10 = OpConstantComposite %f32vec2 %f32_1 %f32_0
%f32vec2_11 = OpConstantComposite %f32vec2 %f32_1 %f32_1
%f32vec2_hh = OpConstantComposite %f32vec2 %f32_0_5 %f32_0_5

%f32vec3_000 = OpConstantComposite %f32vec3 %f32_0 %f32_0 %f32_0
%f32vec3_hhh = OpConstantComposite %f32vec3 %f32_0_5 %f32_0_5 %f32_0_5

%f32vec4_0000 = OpConstantComposite %f32vec4 %f32_0 %f32_0 %f32_0 %f32_0

%type_image_f32_2d_0001 = OpTypeImage %f32 2D 0 0 0 1 Unknown
%ptr_image_f32_2d_0001 = OpTypePointer UniformConstant %type_image_f32_2d_0001
%uniform_image_f32_2d_0001 = OpVariable %ptr_image_f32_2d_0001 UniformConstant
%type_sampled_image_f32_2d_0001 = OpTypeSampledImage %type_image_f32_2d_0001

%type_image_f32_2d_0010 = OpTypeImage %f32 2D 0 0 1 0 Unknown
%ptr_image_f32_2d_0010 = OpTypePointer UniformConstant %type_image_f32_2d_0010
%uniform_image_f32_2d_0010 = OpVariable %ptr_image_f32_2d_0010 UniformConstant
%type_sampled_image_f32_2d_0010 = OpTypeSampledImage %type_image_f32_2d_0010

%type_image_f32_3d_0010 = OpTypeImage %f32 3D 0 0 1 0 Unknown
%ptr_image_f32_3d_0010 = OpTypePointer UniformConstant %type_image_f32_3d_0010
%uniform_image_f32_3d_0010 = OpVariable %ptr_image_f32_3d_0010 UniformConstant
%type_sampled_image_f32_3d_0010 = OpTypeSampledImage %type_image_f32_3d_0010

%type_image_f32_rect_0001 = OpTypeImage %f32 Rect 0 0 0 1 Unknown
%ptr_image_f32_rect_0001 = OpTypePointer UniformConstant %type_image_f32_rect_0001
%uniform_image_f32_rect_0001 = OpVariable %ptr_image_f32_rect_0001 UniformConstant
%type_sampled_image_f32_rect_0001 = OpTypeSampledImage %type_image_f32_rect_0001

%type_sampler = OpTypeSampler
%ptr_sampler = OpTypePointer UniformConstant %type_sampler
%uniform_sampler = OpVariable %ptr_sampler UniformConstant

%main = OpFunction %void None %func
%main_entry = OpLabel
)";

  ss << body;
  ss << R"(
OpReturn
OpFunctionEnd)";

  return ss.str();
}

std::string GetShaderHeader(
    const std::string& capabilities_and_extensions = "") {
  std::ostringstream ss;
  ss << R"(
OpCapability Shader
)";

  ss << capabilities_and_extensions;

  ss << R"(
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main"
%void = OpTypeVoid
%func = OpTypeFunction %void
%bool = OpTypeBool
%f32 = OpTypeFloat 32
%u32 = OpTypeInt 32 0
%s32 = OpTypeInt 32 1
)";

  return ss.str();
}

TEST_F(ValidateImage, TypeImageWrongSampledType) {
  const std::string code = GetShaderHeader() +  R"(
%img_type = OpTypeImage %bool 2D 0 0 0 1 Unknown
)";

  CompileSuccessfully(code.c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(), HasSubstr(
      "TypeImage: expected Sampled Type to be either void or numerical scalar "
      "type"));
}

TEST_F(ValidateImage, TypeImageWrongDepth) {
  const std::string code = GetShaderHeader() +  R"(
%img_type = OpTypeImage %f32 2D 3 0 0 1 Unknown
)";

  CompileSuccessfully(code.c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(), HasSubstr(
      "TypeImage: invalid Depth 3 (must be 0, 1 or 2)"));
}

TEST_F(ValidateImage, TypeImageWrongArrayed) {
  const std::string code = GetShaderHeader() +  R"(
%img_type = OpTypeImage %f32 2D 0 2 0 1 Unknown
)";

  CompileSuccessfully(code.c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(), HasSubstr(
      "TypeImage: invalid Arrayed 2 (must be 0 or 1)"));
}

TEST_F(ValidateImage, TypeImageWrongMS) {
  const std::string code = GetShaderHeader() +  R"(
%img_type = OpTypeImage %f32 2D 0 0 2 1 Unknown
)";

  CompileSuccessfully(code.c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(), HasSubstr(
      "TypeImage: invalid MS 2 (must be 0 or 1)"));
}

TEST_F(ValidateImage, TypeImageWrongSampled) {
  const std::string code = GetShaderHeader() +  R"(
%img_type = OpTypeImage %f32 2D 0 0 0 3 Unknown
)";

  CompileSuccessfully(code.c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(), HasSubstr(
      "TypeImage: invalid Sampled 3 (must be 0, 1 or 2)"));
}

TEST_F(ValidateImage, TypeImageWrongSampledForSubpassData) {
  const std::string code = GetShaderHeader("OpCapability InputAttachment\n") +
      R"(
%img_type = OpTypeImage %f32 SubpassData 0 0 0 1 Unknown
)";

  CompileSuccessfully(code.c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(), HasSubstr(
      "TypeImage: Dim SubpassData requires Sampled to be 2"));
}

TEST_F(ValidateImage, TypeImageWrongFormatForSubpassData) {
  const std::string code = GetShaderHeader("OpCapability InputAttachment\n") +
      R"(
%img_type = OpTypeImage %f32 SubpassData 0 0 0 2 Rgba32f
)";

  CompileSuccessfully(code.c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(), HasSubstr(
      "TypeImage: Dim SubpassData requires format Unknown"));
}

TEST_F(ValidateImage, TypeSampledImageNotImage) {
  const std::string code = GetShaderHeader() +  R"(
%simg_type = OpTypeSampledImage %f32
)";

  CompileSuccessfully(code.c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(), HasSubstr(
      "TypeSampledImage: expected Image to be of type OpTypeImage"));
}

TEST_F(ValidateImage, SampledImageSuccess) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, SampledImageWrongResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_image_f32_2d_0001 %img %sampler
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Result Type to be OpTypeSampledImage: SampledImage"));
}

TEST_F(ValidateImage, SampledImageNotImage) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg1 = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%simg2 = OpSampledImage %type_sampled_image_f32_2d_0001 %simg1 %sampler
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image to be of type OpTypeImage: SampledImage"));
}

TEST_F(ValidateImage, SampledImageImageNotForSampling) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0002 %uniform_image_f32_2d_0002
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0002 %img %sampler
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Expected Image 'Sampled' parameter to be 0 or 1: SampledImage"));
}

TEST_F(ValidateImage, SampledImageNotSampler) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %img
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Sampler to be of type OpTypeSampler: SampledImage"));
}

TEST_F(ValidateImage, SampleImplicitLodSuccess) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec2_hh
%res2 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec2_hh Bias %f32_0_25
%res4 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec2_hh ConstOffset %s32vec2_01
%res5 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec2_hh Offset %s32vec2_01
%res6 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec2_hh MinLod %f32_0_5
%res7 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec2_hh Bias|Offset|MinLod %f32_0_25 %s32vec2_01 %f32_0_5
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, SampleImplicitLodWrongResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleImplicitLod %f32 %simg %f32vec2_hh
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Result Type to be int or float vector type: "
                        "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleImplicitLodWrongNumComponentsResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleImplicitLod %f32vec3 %simg %f32vec2_hh
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Result Type to have 4 components: "
                        "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleImplicitLodNotSampledImage) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageSampleImplicitLod %f32vec4 %img %f32vec2_hh
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Sampled Image to be of type OpTypeSampledImage: "
                "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleImplicitLodWrongSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleImplicitLod %u32vec4 %simg %f32vec2_00
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Image 'Sampled Type' to be the same as "
                        "Result Type components: "
                        "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleImplicitLodVoidSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_void_2d_0001 %uniform_image_void_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_void_2d_0001 %img %sampler
%res1 = OpImageSampleImplicitLod %u32vec4 %simg %f32vec2_00
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, SampleImplicitLodWrongCoordinateType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleImplicitLod %f32vec4 %simg %img
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to be float scalar or vector: "
                        "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleImplicitLodCoordinateSizeTooSmall) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleImplicitLod %f32vec4 %simg %f32_0_5
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to have at least 2 components, "
                        "but given only 1: "
                        "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleExplicitLodSuccessShader) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleExplicitLod %f32vec4 %simg %f32vec4_0000 Lod %f32_1
%res2 = OpImageSampleExplicitLod %f32vec4 %simg %f32vec2_hh Grad %f32vec2_10 %f32vec2_01
%res3 = OpImageSampleExplicitLod %f32vec4 %simg %f32vec2_hh ConstOffset %s32vec2_01
%res4 = OpImageSampleExplicitLod %f32vec4 %simg %f32vec3_hhh Offset %s32vec2_01
%res5 = OpImageSampleExplicitLod %f32vec4 %simg %f32vec2_hh Grad|Offset|MinLod %f32vec2_10 %f32vec2_01 %s32vec2_01 %f32_0_5
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, SampleExplicitLodSuccessKernel) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleExplicitLod %f32vec4 %simg %u32vec4_0123 Lod %f32_1
%res2 = OpImageSampleExplicitLod %f32vec4 %simg %u32vec2_01 Grad %f32vec2_10 %f32vec2_01
%res3 = OpImageSampleExplicitLod %f32vec4 %simg %f32vec2_hh ConstOffset %u32vec2_01
%res4 = OpImageSampleExplicitLod %f32vec4 %simg %u32vec2_01 Offset %u32vec2_01
%res5 = OpImageSampleExplicitLod %f32vec4 %simg %f32vec2_hh Grad|Offset %f32vec2_10 %f32vec2_01 %u32vec2_01
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, SampleExplicitLodSuccessCubeArrayed) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0101 %uniform_image_f32_cube_0101
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_cube_0101 %img %sampler
%res1 = OpImageSampleExplicitLod %f32vec4 %simg %f32vec4_0000 Grad %f32vec3_hhh %f32vec3_hhh
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, SampleExplicitLodWrongResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleExplicitLod %f32 %simg %f32vec2_hh Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Result Type to be int or float vector type: "
                        "ImageSampleExplicitLod"));
}

TEST_F(ValidateImage, SampleExplicitLodWrongNumComponentsResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleExplicitLod %f32vec3 %simg %f32vec2_hh Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Result Type to have 4 components: "
                        "ImageSampleExplicitLod"));
}

TEST_F(ValidateImage, SampleExplicitLodNotSampledImage) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageSampleExplicitLod %f32vec4 %img %f32vec2_hh Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Sampled Image to be of type OpTypeSampledImage: "
                "ImageSampleExplicitLod"));
}

TEST_F(ValidateImage, SampleExplicitLodWrongSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleExplicitLod %u32vec4 %simg %f32vec2_00 Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Image 'Sampled Type' to be the same as "
                        "Result Type components: "
                        "ImageSampleExplicitLod"));
}

TEST_F(ValidateImage, SampleExplicitLodVoidSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_void_2d_0001 %uniform_image_void_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_void_2d_0001 %img %sampler
%res1 = OpImageSampleExplicitLod %u32vec4 %simg %f32vec2_00 Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, SampleExplicitLodWrongCoordinateType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleExplicitLod %f32vec4 %simg %img Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to be float scalar or vector: "
                        "ImageSampleExplicitLod"));
}

TEST_F(ValidateImage, SampleExplicitLodCoordinateSizeTooSmall) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleExplicitLod %f32vec4 %simg %f32_0_5 Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to have at least 2 components, "
                        "but given only 1: "
                        "ImageSampleExplicitLod"));
}

TEST_F(ValidateImage, SampleExplicitLodBias) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleExplicitLod %f32vec4 %simg %f32vec2_00 Bias|Lod %f32_1 %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Image Operand Bias can only be used with ImplicitLod opcodes: "
                "ImageSampleExplicitLod"));
}

TEST_F(ValidateImage, LodAndGrad) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleExplicitLod %f32vec4 %simg %f32vec2_00 Lod|Grad %f32_1 %f32vec2_hh %f32vec2_hh
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Image Operand bits Lod and Grad cannot be set at the same time: "
          "ImageSampleExplicitLod"));
}

TEST_F(ValidateImage, ImplicitLodWithLod) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res2 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec2_hh Lod %f32_0_5
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Image Operand Lod cannot be used with ImplicitLod opcodes: "
                "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, LodWrongType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleExplicitLod %f32vec4 %simg %f32vec2_00 Lod %f32vec2_hh)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Image Operand Lod to be int or float scalar: "
                        "ImageSampleExplicitLod"));
}

TEST_F(ValidateImage, LodWrongDim) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_rect_0001 %uniform_image_f32_rect_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_rect_0001 %img %sampler
%res1 = OpImageSampleExplicitLod %f32vec4 %simg %f32vec2_00 Lod %f32_0)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Image Operand Lod requires 'Dim' parameter to be 1D, "
                        "2D, 3D or Cube: "
                        "ImageSampleExplicitLod"));
}

TEST_F(ValidateImage, LodMultisampled) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0010 %uniform_image_f32_2d_0010
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0010 %img %sampler
%res1 = OpImageSampleExplicitLod %f32vec4 %simg %f32vec2_00 Lod %f32_0)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Image Operand Lod requires 'MS' parameter to be 0: "
                        "ImageSampleExplicitLod"));
}

TEST_F(ValidateImage, MinLodIncompatible) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleExplicitLod %f32vec4 %simg %f32vec2_00 Lod|MinLod %f32_0 %f32_0)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Image Operand MinLod can only be used with ImplicitLod opcodes or "
          "together with Image Operand Grad: ImageSampleExplicitLod"));
}

TEST_F(ValidateImage, ImplicitLodWithGrad) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res2 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec2_hh Grad %f32vec2_hh %f32vec2_hh
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Image Operand Grad can only be used with ExplicitLod opcodes: "
                "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleImplicitLod3DArrayedMultisampledSuccess) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_3d_0111 %uniform_image_f32_3d_0111
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_3d_0111 %img %sampler
%res1 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec4_0000
%res2 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec4_0000 ConstOffset %s32vec3_012
%res3 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec4_0000 Offset %s32vec3_012
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, SampleImplicitLodCubeArrayedSuccess) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0101 %uniform_image_f32_cube_0101
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_cube_0101 %img %sampler
%res1 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec4_0000
%res2 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec4_0000 Bias %f32_0_25
%res4 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec4_0000 MinLod %f32_0_5
%res5 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec4_0000 Bias|MinLod %f32_0_25 %f32_0_5
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, SampleImplicitLodBiasWrongType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res2 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec2_hh Bias %u32_0
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Image Operand Bias to be float scalar: "
                        "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleImplicitLodBiasWrongDim) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_rect_0001 %uniform_image_f32_rect_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_rect_0001 %img %sampler
%res2 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec2_hh Bias %f32_0
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Image Operand Bias requires 'Dim' parameter to be 1D, "
                        "2D, 3D or Cube: "
                        "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleImplicitLodBiasMultisampled) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_3d_0111 %uniform_image_f32_3d_0111
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_3d_0111 %img %sampler
%res1 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec4_0000 Bias %f32_0_25
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Image Operand Bias requires 'MS' parameter to be 0: "
                        "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleExplicitLodGradDxWrongType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0101 %uniform_image_f32_cube_0101
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_cube_0101 %img %sampler
%res1 = OpImageSampleExplicitLod %f32vec4 %simg %f32vec4_0000 Grad %s32vec3_012 %f32vec3_hhh
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected both Image Operand Grad ids to be float "
                        "scalars or vectors: "
                        "ImageSampleExplicitLod"));
}

TEST_F(ValidateImage, SampleExplicitLodGradDyWrongType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0101 %uniform_image_f32_cube_0101
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_cube_0101 %img %sampler
%res1 = OpImageSampleExplicitLod %f32vec4 %simg %f32vec4_0000 Grad  %f32vec3_hhh %s32vec3_012
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected both Image Operand Grad ids to be float "
                        "scalars or vectors: "
                        "ImageSampleExplicitLod"));
}

TEST_F(ValidateImage, SampleExplicitLodGradDxWrongSize) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0101 %uniform_image_f32_cube_0101
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_cube_0101 %img %sampler
%res1 = OpImageSampleExplicitLod %f32vec4 %simg %f32vec4_0000 Grad %f32vec2_00 %f32vec3_hhh
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Expected Image Operand Grad dx to have 3 components, but given 2: "
          "ImageSampleExplicitLod"));
}

TEST_F(ValidateImage, SampleExplicitLodGradDyWrongSize) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0101 %uniform_image_f32_cube_0101
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_cube_0101 %img %sampler
%res1 = OpImageSampleExplicitLod %f32vec4 %simg %f32vec4_0000 Grad %f32vec3_hhh %f32vec2_00
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Expected Image Operand Grad dy to have 3 components, but given 2: "
          "ImageSampleExplicitLod"));
}

TEST_F(ValidateImage, SampleExplicitLodGradMultisampled) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_3d_0111 %uniform_image_f32_3d_0111
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_3d_0111 %img %sampler
%res1 = OpImageSampleExplicitLod %f32vec4 %simg %f32vec4_0000 Grad %f32vec3_000 %f32vec3_000
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Image Operand Grad requires 'MS' parameter to be 0: "
                        "ImageSampleExplicitLod"));
}

TEST_F(ValidateImage, SampleImplicitLodConstOffsetCubeDim) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0101 %uniform_image_f32_cube_0101
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_cube_0101 %img %sampler
%res4 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec4_0000 ConstOffset %s32vec3_012
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Image Operand ConstOffset cannot be used with Cube Image 'Dim': "
          "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleImplicitLodConstOffsetWrongType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_3d_0111 %uniform_image_f32_3d_0111
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_3d_0111 %img %sampler
%res4 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec4_0000 ConstOffset %f32vec3_000
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Expected Image Operand ConstOffset to be int scalar or vector: "
          "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleImplicitLodConstOffsetWrongSize) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_3d_0111 %uniform_image_f32_3d_0111
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_3d_0111 %img %sampler
%res4 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec4_0000 ConstOffset %s32vec2_01
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Image Operand ConstOffset to have 3 "
                        "components, but given 2: "
                        "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleImplicitLodConstOffsetNotConst) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_3d_0111 %uniform_image_f32_3d_0111
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_3d_0111 %img %sampler
%offset = OpSNegate %s32vec3 %s32vec3_012
%res4 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec4_0000 ConstOffset %offset
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image Operand ConstOffset to be a const object: "
                "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleImplicitLodOffsetCubeDim) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0101 %uniform_image_f32_cube_0101
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_cube_0101 %img %sampler
%res4 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec4_0000 Offset %s32vec3_012
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Image Operand Offset cannot be used with Cube Image 'Dim': "
                "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleImplicitLodOffsetWrongType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_3d_0111 %uniform_image_f32_3d_0111
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_3d_0111 %img %sampler
%res4 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec4_0000 Offset %f32vec3_000
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image Operand Offset to be int scalar or vector: "
                "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleImplicitLodOffsetWrongSize) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_3d_0111 %uniform_image_f32_3d_0111
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_3d_0111 %img %sampler
%res4 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec4_0000 Offset %s32vec2_01
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Expected Image Operand Offset to have 3 components, but given 2: "
          "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleImplicitLodMoreThanOneOffset) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_3d_0111 %uniform_image_f32_3d_0111
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_3d_0111 %img %sampler
%res4 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec4_0000 ConstOffset|Offset %s32vec3_012 %s32vec3_012
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Image Operands Offset, ConstOffset, ConstOffsets "
                        "cannot be used together: "
                        "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleImplicitLodMinLodWrongType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0101 %uniform_image_f32_cube_0101
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_cube_0101 %img %sampler
%res1 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec4_0000 MinLod %s32_0
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Image Operand MinLod to be float scalar: "
                        "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleImplicitLodMinLodWrongDim) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_rect_0001 %uniform_image_f32_rect_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_rect_0001 %img %sampler
%res2 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec2_hh MinLod %f32_0_25
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Image Operand MinLod requires 'Dim' parameter to be "
                        "1D, 2D, 3D or Cube: "
                        "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleImplicitLodMinLodMultisampled) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_3d_0111 %uniform_image_f32_3d_0111
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_3d_0111 %img %sampler
%res1 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec4_0000 MinLod %f32_0_25
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Image Operand MinLod requires 'MS' parameter to be 0: "
                        "ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, SampleProjExplicitLodSuccess2D) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleProjExplicitLod %f32vec4 %simg %f32vec3_hhh Lod %f32_1
%res3 = OpImageSampleProjExplicitLod %f32vec4 %simg %f32vec3_hhh Grad %f32vec2_10 %f32vec2_01
%res4 = OpImageSampleProjExplicitLod %f32vec4 %simg %f32vec3_hhh ConstOffset %s32vec2_01
%res5 = OpImageSampleProjExplicitLod %f32vec4 %simg %f32vec3_hhh Offset %s32vec2_01
%res7 = OpImageSampleProjExplicitLod %f32vec4 %simg %f32vec3_hhh Grad|Offset %f32vec2_10 %f32vec2_01 %s32vec2_01
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, SampleProjExplicitLodSuccessRect) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_rect_0001 %uniform_image_f32_rect_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_rect_0001 %img %sampler
%res1 = OpImageSampleProjExplicitLod %f32vec4 %simg %f32vec3_hhh Grad %f32vec2_10 %f32vec2_01
%res2 = OpImageSampleProjExplicitLod %f32vec4 %simg %f32vec3_hhh Grad|Offset %f32vec2_10 %f32vec2_01 %s32vec2_01
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, SampleProjExplicitLodWrongResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleProjExplicitLod %f32 %simg %f32vec3_hhh Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Result Type to be int or float vector type: "
                        "ImageSampleProjExplicitLod"));
}

TEST_F(ValidateImage, SampleProjExplicitLodWrongNumComponentsResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleProjExplicitLod %f32vec3 %simg %f32vec3_hhh Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Result Type to have 4 components: "
                        "ImageSampleProjExplicitLod"));
}

TEST_F(ValidateImage, SampleProjExplicitLodNotSampledImage) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageSampleProjExplicitLod %f32vec4 %img %f32vec3_hhh Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Sampled Image to be of type OpTypeSampledImage: "
                "ImageSampleProjExplicitLod"));
}

TEST_F(ValidateImage, SampleProjExplicitLodWrongSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleProjExplicitLod %u32vec4 %simg %f32vec3_hhh Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Image 'Sampled Type' to be the same as "
                        "Result Type components: "
                        "ImageSampleProjExplicitLod"));
}

TEST_F(ValidateImage, SampleProjExplicitLodVoidSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_void_2d_0001 %uniform_image_void_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_void_2d_0001 %img %sampler
%res1 = OpImageSampleProjExplicitLod %u32vec4 %simg %f32vec3_hhh Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, SampleProjExplicitLodWrongCoordinateType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleProjExplicitLod %f32vec4 %simg %img Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to be float scalar or vector: "
                        "ImageSampleProjExplicitLod"));
}

TEST_F(ValidateImage, SampleProjExplicitLodCoordinateSizeTooSmall) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleProjExplicitLod %f32vec4 %simg %f32vec2_hh Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to have at least 3 components, "
                        "but given only 2: "
                        "ImageSampleProjExplicitLod"));
}

TEST_F(ValidateImage, SampleProjImplicitLodSuccess) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleProjImplicitLod %f32vec4 %simg %f32vec3_hhh
%res2 = OpImageSampleProjImplicitLod %f32vec4 %simg %f32vec3_hhh Bias %f32_0_25
%res4 = OpImageSampleProjImplicitLod %f32vec4 %simg %f32vec3_hhh ConstOffset %s32vec2_01
%res5 = OpImageSampleProjImplicitLod %f32vec4 %simg %f32vec3_hhh Offset %s32vec2_01
%res6 = OpImageSampleProjImplicitLod %f32vec4 %simg %f32vec3_hhh MinLod %f32_0_5
%res7 = OpImageSampleProjImplicitLod %f32vec4 %simg %f32vec3_hhh Bias|Offset|MinLod %f32_0_25 %s32vec2_01 %f32_0_5
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, SampleProjImplicitLodWrongResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleProjImplicitLod %f32 %simg %f32vec3_hhh
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Result Type to be int or float vector type: "
                        "ImageSampleProjImplicitLod"));
}

TEST_F(ValidateImage, SampleProjImplicitLodWrongNumComponentsResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleProjImplicitLod %f32vec3 %simg %f32vec3_hhh
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Result Type to have 4 components: "
                        "ImageSampleProjImplicitLod"));
}

TEST_F(ValidateImage, SampleProjImplicitLodNotSampledImage) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageSampleProjImplicitLod %f32vec4 %img %f32vec3_hhh
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Sampled Image to be of type OpTypeSampledImage: "
                "ImageSampleProjImplicitLod"));
}

TEST_F(ValidateImage, SampleProjImplicitLodWrongSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleProjImplicitLod %u32vec4 %simg %f32vec3_hhh
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Image 'Sampled Type' to be the same as "
                        "Result Type components: "
                        "ImageSampleProjImplicitLod"));
}

TEST_F(ValidateImage, SampleProjImplicitLodVoidSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_void_2d_0001 %uniform_image_void_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_void_2d_0001 %img %sampler
%res1 = OpImageSampleProjImplicitLod %u32vec4 %simg %f32vec3_hhh
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, SampleProjImplicitLodWrongCoordinateType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleProjImplicitLod %f32vec4 %simg %img
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to be float scalar or vector: "
                        "ImageSampleProjImplicitLod"));
}

TEST_F(ValidateImage, SampleProjImplicitLodCoordinateSizeTooSmall) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleProjImplicitLod %f32vec4 %simg %f32vec2_hh
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to have at least 3 components, "
                        "but given only 2: "
                        "ImageSampleProjImplicitLod"));
}

TEST_F(ValidateImage, SampleDrefImplicitLodSuccess) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0001 %uniform_image_u32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_u32_2d_0001 %img %sampler
%res1 = OpImageSampleDrefImplicitLod %u32 %simg %f32vec2_hh %f32_1
%res2 = OpImageSampleDrefImplicitLod %u32 %simg %f32vec2_hh %f32_1 Bias %f32_0_25
%res4 = OpImageSampleDrefImplicitLod %u32 %simg %f32vec2_hh %f32_1 ConstOffset %s32vec2_01
%res5 = OpImageSampleDrefImplicitLod %u32 %simg %f32vec2_hh %f32_1 Offset %s32vec2_01
%res6 = OpImageSampleDrefImplicitLod %u32 %simg %f32vec2_hh %f32_1 MinLod %f32_0_5
%res7 = OpImageSampleDrefImplicitLod %u32 %simg %f32vec2_hh %f32_1 Bias|Offset|MinLod %f32_0_25 %s32vec2_01 %f32_0_5
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, SampleDrefImplicitLodWrongResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_void_2d_0001 %uniform_image_void_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_void_2d_0001 %img %sampler
%res1 = OpImageSampleDrefImplicitLod %void %simg %f32vec2_hh %u32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Result Type to be int or float scalar type: "
                        "ImageSampleDrefImplicitLod"));
}

TEST_F(ValidateImage, SampleDrefImplicitLodNotSampledImage) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0001 %uniform_image_u32_2d_0001
%res1 = OpImageSampleDrefImplicitLod %u32 %img %f32vec2_hh %u32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Sampled Image to be of type OpTypeSampledImage: "
                "ImageSampleDrefImplicitLod"));
}

TEST_F(ValidateImage, SampleDrefImplicitLodWrongSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0001 %uniform_image_u32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_u32_2d_0001 %img %sampler
%res1 = OpImageSampleDrefImplicitLod %f32 %simg %f32vec2_00 %u32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image 'Sampled Type' to be the same as Result Type: "
                "ImageSampleDrefImplicitLod"));
}

TEST_F(ValidateImage, SampleDrefImplicitLodVoidSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_void_2d_0001 %uniform_image_void_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_void_2d_0001 %img %sampler
%res1 = OpImageSampleDrefImplicitLod %u32 %simg %f32vec2_00 %u32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image 'Sampled Type' to be the same as Result Type: "
                "ImageSampleDrefImplicitLod"));
}

TEST_F(ValidateImage, SampleDrefImplicitLodWrongCoordinateType) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0001 %uniform_image_u32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_u32_2d_0001 %img %sampler
%res1 = OpImageSampleDrefImplicitLod %u32 %simg %img %u32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to be float scalar or vector: "
                        "ImageSampleDrefImplicitLod"));
}

TEST_F(ValidateImage, SampleDrefImplicitLodCoordinateSizeTooSmall) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleDrefImplicitLod %f32 %simg %f32_0_5 %f32_0_5
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to have at least 2 components, "
                        "but given only 1: "
                        "ImageSampleDrefImplicitLod"));
}

TEST_F(ValidateImage, SampleDrefImplicitLodWrongDrefType) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0001 %uniform_image_u32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_u32_2d_0001 %img %sampler
%res1 = OpImageSampleDrefImplicitLod %u32 %simg %f32vec2_00 %f16_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("ImageSampleDrefImplicitLod: "
                        "Expected Dref to be of 32-bit float type"));
}

TEST_F(ValidateImage, SampleDrefExplicitLodSuccess) {
  const std::string body = R"(
%img = OpLoad %type_image_s32_3d_0001 %uniform_image_s32_3d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_s32_3d_0001 %img %sampler
%res1 = OpImageSampleDrefExplicitLod %s32 %simg %f32vec4_0000 %f32_1 Lod %f32_1
%res3 = OpImageSampleDrefExplicitLod %s32 %simg %f32vec3_hhh %f32_1 Grad %f32vec3_hhh %f32vec3_hhh
%res4 = OpImageSampleDrefExplicitLod %s32 %simg %f32vec3_hhh %f32_1 ConstOffset %s32vec3_012
%res5 = OpImageSampleDrefExplicitLod %s32 %simg %f32vec4_0000 %f32_1 Offset %s32vec3_012
%res7 = OpImageSampleDrefExplicitLod %s32 %simg %f32vec3_hhh %f32_1 Grad|Offset %f32vec3_hhh %f32vec3_hhh %s32vec3_012
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, SampleDrefExplicitLodWrongResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_s32_3d_0001 %uniform_image_s32_3d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_s32_3d_0001 %img %sampler
%res1 = OpImageSampleDrefExplicitLod %bool %simg %f32vec3_hhh %s32_1 Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Result Type to be int or float scalar type: "
                        "ImageSampleDrefExplicitLod"));
}

TEST_F(ValidateImage, SampleDrefExplicitLodNotSampledImage) {
  const std::string body = R"(
%img = OpLoad %type_image_s32_3d_0001 %uniform_image_s32_3d_0001
%res1 = OpImageSampleDrefExplicitLod %s32 %img %f32vec3_hhh %s32_1 Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Sampled Image to be of type OpTypeSampledImage: "
                "ImageSampleDrefExplicitLod"));
}

TEST_F(ValidateImage, SampleDrefExplicitLodWrongSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_s32_3d_0001 %uniform_image_s32_3d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_s32_3d_0001 %img %sampler
%res1 = OpImageSampleDrefExplicitLod %f32 %simg %f32vec3_hhh %s32_1 Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image 'Sampled Type' to be the same as Result Type: "
                "ImageSampleDrefExplicitLod"));
}

TEST_F(ValidateImage, SampleDrefExplicitLodVoidSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_void_2d_0001 %uniform_image_void_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_void_2d_0001 %img %sampler
%res1 = OpImageSampleDrefExplicitLod %u32 %simg %f32vec2_00 %s32_1 Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image 'Sampled Type' to be the same as Result Type: "
                "ImageSampleDrefExplicitLod"));
}

TEST_F(ValidateImage, SampleDrefExplicitLodWrongCoordinateType) {
  const std::string body = R"(
%img = OpLoad %type_image_s32_3d_0001 %uniform_image_s32_3d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_s32_3d_0001 %img %sampler
%res1 = OpImageSampleDrefExplicitLod %s32 %simg %img %s32_1 Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to be float scalar or vector: "
                        "ImageSampleDrefExplicitLod"));
}

TEST_F(ValidateImage, SampleDrefExplicitLodCoordinateSizeTooSmall) {
  const std::string body = R"(
%img = OpLoad %type_image_s32_3d_0001 %uniform_image_s32_3d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_s32_3d_0001 %img %sampler
%res1 = OpImageSampleDrefExplicitLod %s32 %simg %f32vec2_hh %s32_1 Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to have at least 3 components, "
                        "but given only 2: "
                        "ImageSampleDrefExplicitLod"));
}

TEST_F(ValidateImage, SampleDrefExplicitLodWrongDrefType) {
  const std::string body = R"(
%img = OpLoad %type_image_s32_3d_0001 %uniform_image_s32_3d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_s32_3d_0001 %img %sampler
%res1 = OpImageSampleDrefExplicitLod %s32 %simg %f32vec3_hhh %u32_1 Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("ImageSampleDrefExplicitLod: "
                        "Expected Dref to be of 32-bit float type"));
}

TEST_F(ValidateImage, SampleProjDrefImplicitLodSuccess) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleProjDrefImplicitLod %f32 %simg %f32vec3_hhh %f32_0_5
%res2 = OpImageSampleProjDrefImplicitLod %f32 %simg %f32vec3_hhh %f32_0_5 Bias %f32_0_25
%res4 = OpImageSampleProjDrefImplicitLod %f32 %simg %f32vec3_hhh %f32_0_5 ConstOffset %s32vec2_01
%res5 = OpImageSampleProjDrefImplicitLod %f32 %simg %f32vec3_hhh %f32_0_5 Offset %s32vec2_01
%res6 = OpImageSampleProjDrefImplicitLod %f32 %simg %f32vec3_hhh %f32_0_5 MinLod %f32_0_5
%res7 = OpImageSampleProjDrefImplicitLod %f32 %simg %f32vec3_hhh %f32_0_5 Bias|Offset|MinLod %f32_0_25 %s32vec2_01 %f32_0_5
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, SampleProjDrefImplicitLodWrongResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleProjDrefImplicitLod %void %simg %f32vec3_hhh %f32_0_5
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Result Type to be int or float scalar type: "
                        "ImageSampleProjDrefImplicitLod"));
}

TEST_F(ValidateImage, SampleProjDrefImplicitLodNotSampledImage) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageSampleProjDrefImplicitLod %f32 %img %f32vec3_hhh %f32_0_5
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Sampled Image to be of type OpTypeSampledImage: "
                "ImageSampleProjDrefImplicitLod"));
}

TEST_F(ValidateImage, SampleProjDrefImplicitLodWrongSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleProjDrefImplicitLod %u32 %simg %f32vec3_hhh %f32_0_5
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image 'Sampled Type' to be the same as Result Type: "
                "ImageSampleProjDrefImplicitLod"));
}

TEST_F(ValidateImage, SampleProjDrefImplicitLodVoidSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_void_2d_0001 %uniform_image_void_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_void_2d_0001 %img %sampler
%res1 = OpImageSampleProjDrefImplicitLod %u32 %simg %f32vec3_hhh %f32_0_5
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image 'Sampled Type' to be the same as Result Type: "
                "ImageSampleProjDrefImplicitLod"));
}

TEST_F(ValidateImage, SampleProjDrefImplicitLodWrongCoordinateType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleProjDrefImplicitLod %f32 %simg %img %f32_0_5
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to be float scalar or vector: "
                        "ImageSampleProjDrefImplicitLod"));
}

TEST_F(ValidateImage, SampleProjDrefImplicitLodCoordinateSizeTooSmall) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleProjDrefImplicitLod %f32 %simg %f32vec2_hh %f32_0_5
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to have at least 3 components, "
                        "but given only 2: "
                        "ImageSampleProjDrefImplicitLod"));
}

TEST_F(ValidateImage, SampleProjDrefImplicitLodWrongDrefType) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0001 %uniform_image_u32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_u32_2d_0001 %img %sampler
%res1 = OpImageSampleProjDrefImplicitLod %u32 %simg %f32vec3_hhh %f32vec4_0000
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("ImageSampleProjDrefImplicitLod: "
                        "Expected Dref to be of 32-bit float type"));
}

TEST_F(ValidateImage, SampleProjDrefExplicitLodSuccess) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_1d_0001 %uniform_image_f32_1d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_1d_0001 %img %sampler
%res1 = OpImageSampleProjDrefExplicitLod %f32 %simg %f32vec2_hh %f32_0_5 Lod %f32_1
%res2 = OpImageSampleProjDrefExplicitLod %f32 %simg %f32vec3_hhh %f32_0_5 Grad %f32_0_5 %f32_0_5
%res3 = OpImageSampleProjDrefExplicitLod %f32 %simg %f32vec2_hh %f32_0_5 ConstOffset %s32_1
%res4 = OpImageSampleProjDrefExplicitLod %f32 %simg %f32vec2_hh %f32_0_5 Offset %s32_1
%res5 = OpImageSampleProjDrefExplicitLod %f32 %simg %f32vec2_hh %f32_0_5 Grad|Offset %f32_0_5 %f32_0_5 %s32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, SampleProjDrefExplicitLodWrongResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_1d_0001 %uniform_image_f32_1d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_1d_0001 %img %sampler
%res1 = OpImageSampleProjDrefExplicitLod %bool %simg %f32vec2_hh %f32_0_5 Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Result Type to be int or float scalar type: "
                        "ImageSampleProjDrefExplicitLod"));
}

TEST_F(ValidateImage, SampleProjDrefExplicitLodNotSampledImage) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_1d_0001 %uniform_image_f32_1d_0001
%res1 = OpImageSampleProjDrefExplicitLod %f32 %img %f32vec2_hh %f32_0_5 Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Sampled Image to be of type OpTypeSampledImage: "
                "ImageSampleProjDrefExplicitLod"));
}

TEST_F(ValidateImage, SampleProjDrefExplicitLodWrongSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_1d_0001 %uniform_image_f32_1d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_1d_0001 %img %sampler
%res1 = OpImageSampleProjDrefExplicitLod %u32 %simg %f32vec2_hh %f32_0_5 Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image 'Sampled Type' to be the same as Result Type: "
                "ImageSampleProjDrefExplicitLod"));
}

TEST_F(ValidateImage, SampleProjDrefExplicitLodVoidSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_void_2d_0001 %uniform_image_void_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_void_2d_0001 %img %sampler
%res1 = OpImageSampleProjDrefExplicitLod %u32 %simg %f32vec3_hhh %f32_0_5 Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image 'Sampled Type' to be the same as Result Type: "
                "ImageSampleProjDrefExplicitLod"));
}

TEST_F(ValidateImage, SampleProjDrefExplicitLodWrongCoordinateType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_1d_0001 %uniform_image_f32_1d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_1d_0001 %img %sampler
%res1 = OpImageSampleProjDrefExplicitLod %f32 %simg %img %f32_0_5 Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to be float scalar or vector: "
                        "ImageSampleProjDrefExplicitLod"));
}

TEST_F(ValidateImage, SampleProjDrefExplicitLodCoordinateSizeTooSmall) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_1d_0001 %uniform_image_f32_1d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_1d_0001 %img %sampler
%res1 = OpImageSampleProjDrefExplicitLod %f32 %simg %f32_0_5 %f32_0_5 Lod %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to have at least 2 components, "
                        "but given only 1: "
                        "ImageSampleProjDrefExplicitLod"));
}

TEST_F(ValidateImage, FetchSuccess) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_rect_0001 %uniform_image_f32_rect_0001
%res1 = OpImageFetch %f32vec4 %img %u32vec2_01
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, FetchWrongResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_rect_0001 %uniform_image_f32_rect_0001
%res1 = OpImageFetch %f32 %img %u32vec2_01
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Result Type to be int or float vector type: "
                        "ImageFetch"));
}

TEST_F(ValidateImage, FetchWrongNumComponentsResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_rect_0001 %uniform_image_f32_rect_0001
%res1 = OpImageFetch %f32vec3 %img %u32vec2_01
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Result Type to have 4 components: ImageFetch"));
}

TEST_F(ValidateImage, FetchNotImage) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageFetch %f32vec4 %simg %u32vec2_01
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image to be of type OpTypeImage: ImageFetch"));
}

TEST_F(ValidateImage, FetchNotSampled) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0000 %uniform_image_u32_2d_0000
%res1 = OpImageFetch %u32vec4 %img %u32vec2_01
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image 'Sampled' parameter to be 1: ImageFetch"));
}

TEST_F(ValidateImage, FetchCube) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0101 %uniform_image_f32_cube_0101
%res1 = OpImageFetch %f32vec4 %img %u32vec3_012
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Image 'Dim' cannot be Cube: ImageFetch"));
}

TEST_F(ValidateImage, FetchWrongSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_rect_0001 %uniform_image_f32_rect_0001
%res1 = OpImageFetch %u32vec4 %img %u32vec2_01
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Image 'Sampled Type' to be the same as "
                        "Result Type components: "
                        "ImageFetch"));
}

TEST_F(ValidateImage, FetchVoidSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_void_2d_0001 %uniform_image_void_2d_0001
%res1 = OpImageFetch %f32vec4 %img %u32vec2_01
%res2 = OpImageFetch %u32vec4 %img %u32vec2_01
%res3 = OpImageFetch %s32vec4 %img %u32vec2_01
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, FetchWrongCoordinateType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_rect_0001 %uniform_image_f32_rect_0001
%res1 = OpImageFetch %f32vec4 %img %f32vec2_00
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to be int scalar or vector: "
                        "ImageFetch"));
}

TEST_F(ValidateImage, FetchCoordinateSizeTooSmall) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_rect_0001 %uniform_image_f32_rect_0001
%res1 = OpImageFetch %f32vec4 %img %u32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to have at least 2 components, "
                        "but given only 1: "
                        "ImageFetch"));
}

TEST_F(ValidateImage, GatherSuccess) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageGather %f32vec4 %simg %f32vec4_0000 %u32_1
%res2 = OpImageGather %f32vec4 %simg %f32vec4_0000 %u32_1 ConstOffsets %const_offsets
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, GatherWrongResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0101 %uniform_image_f32_cube_0101
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_cube_0101 %img %sampler
%res1 = OpImageGather %f32 %simg %f32vec4_0000 %u32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Result Type to be int or float vector type: "
                        "ImageGather"));
}

TEST_F(ValidateImage, GatherWrongNumComponentsResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0101 %uniform_image_f32_cube_0101
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_cube_0101 %img %sampler
%res1 = OpImageGather %f32vec3 %simg %f32vec4_0000 %u32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Result Type to have 4 components: "
                        "ImageGather"));
}

TEST_F(ValidateImage, GatherNotSampledImage) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0101 %uniform_image_f32_cube_0101
%res1 = OpImageGather %f32vec4 %img %f32vec4_0000 %u32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Sampled Image to be of type OpTypeSampledImage: "
                "ImageGather"));
}

TEST_F(ValidateImage, GatherWrongSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0101 %uniform_image_f32_cube_0101
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_cube_0101 %img %sampler
%res1 = OpImageGather %u32vec4 %simg %f32vec4_0000 %u32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Image 'Sampled Type' to be the same as "
                        "Result Type components: "
                        "ImageGather"));
}

TEST_F(ValidateImage, GatherVoidSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_void_2d_0001 %uniform_image_void_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_void_2d_0001 %img %sampler
%res1 = OpImageGather %u32vec4 %simg %f32vec2_00 %u32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, GatherWrongCoordinateType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0101 %uniform_image_f32_cube_0101
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_cube_0101 %img %sampler
%res1 = OpImageGather %f32vec4 %simg %u32vec4_0123 %u32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to be float scalar or vector: "
                        "ImageGather"));
}

TEST_F(ValidateImage, GatherCoordinateSizeTooSmall) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0101 %uniform_image_f32_cube_0101
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_cube_0101 %img %sampler
%res1 = OpImageGather %f32vec4 %simg %f32_0_5 %u32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to have at least 4 components, "
                        "but given only 1: "
                        "ImageGather"));
}

TEST_F(ValidateImage, GatherWrongComponentType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0101 %uniform_image_f32_cube_0101
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_cube_0101 %img %sampler
%res1 = OpImageGather %f32vec4 %simg %f32vec4_0000 %f32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Component to be int scalar: ImageGather"));
}

TEST_F(ValidateImage, GatherDimCube) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0101 %uniform_image_f32_cube_0101
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_cube_0101 %img %sampler
%res1 = OpImageGather %f32vec4 %simg %f32vec4_0000 %u32_1 ConstOffsets %const_offsets
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Image Operand ConstOffsets cannot be used with Cube Image 'Dim': "
          "ImageGather"));
}

TEST_F(ValidateImage, GatherConstOffsetsNotArray) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageGather %f32vec4 %simg %f32vec4_0000 %u32_1 ConstOffsets %u32vec4_0123
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image Operand ConstOffsets to be an array of size 4: "
                "ImageGather"));
}

TEST_F(ValidateImage, GatherConstOffsetsArrayWrongSize) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageGather %f32vec4 %simg %f32vec4_0000 %u32_1 ConstOffsets %const_offsets3x2
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image Operand ConstOffsets to be an array of size 4: "
                "ImageGather"));
}

TEST_F(ValidateImage, GatherConstOffsetsArrayNotVector) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageGather %f32vec4 %simg %f32vec4_0000 %u32_1 ConstOffsets %const_offsets4xu
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Image Operand ConstOffsets array componenets "
                        "to be int vectors "
                        "of size 2: ImageGather"));
}

TEST_F(ValidateImage, GatherConstOffsetsArrayVectorWrongSize) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageGather %f32vec4 %simg %f32vec4_0000 %u32_1 ConstOffsets %const_offsets4x3
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Image Operand ConstOffsets array componenets "
                        "to be int vectors "
                        "of size 2: ImageGather"));
}

TEST_F(ValidateImage, GatherConstOffsetsArrayNotConst) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%offsets = OpUndef %u32vec2arr4
%res1 = OpImageGather %f32vec4 %simg %f32vec4_0000 %u32_1 ConstOffsets %offsets
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image Operand ConstOffsets to be a const object: "
                "ImageGather"));
}

TEST_F(ValidateImage, NotGatherWithConstOffsets) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res2 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec2_hh ConstOffsets %const_offsets
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Image Operand ConstOffsets can only be used with OpImageGather "
          "and OpImageDrefGather: ImageSampleImplicitLod"));
}

TEST_F(ValidateImage, DrefGatherSuccess) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageDrefGather %f32vec4 %simg %f32vec4_0000 %f32_0_5
%res2 = OpImageDrefGather %f32vec4 %simg %f32vec4_0000 %f32_0_5 ConstOffsets %const_offsets
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, DrefGatherVoidSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_void_2d_0001 %uniform_image_void_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_void_2d_0001 %img %sampler
%res1 = OpImageDrefGather %u32vec4 %simg %f32vec2_00 %f32_0_5
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Image 'Sampled Type' to be the same as "
                        "Result Type components: "
                        "ImageDrefGather"));
}

TEST_F(ValidateImage, DrefGatherWrongDrefType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0101 %uniform_image_f32_cube_0101
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_cube_0101 %img %sampler
%res1 = OpImageDrefGather %f32vec4 %simg %f32vec4_0000 %u32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("ImageDrefGather: "
                        "Expected Dref to be of 32-bit float type"));
}

TEST_F(ValidateImage, ReadSuccess1) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0000 %uniform_image_u32_2d_0000
%res1 = OpImageRead %u32vec4 %img %u32vec2_01
)";

  const std::string extra = "\nOpCapability StorageImageReadWithoutFormat\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, ReadSuccess2) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_1d_0002_rgba32f %uniform_image_f32_1d_0002_rgba32f
%res1 = OpImageRead %f32vec4 %img %u32vec2_01
)";

  const std::string extra = "\nOpCapability Image1D\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, ReadSuccess3) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0102_rgba32f %uniform_image_f32_cube_0102_rgba32f
%res1 = OpImageRead %f32vec4 %img %u32vec3_012
)";

  const std::string extra = "\nOpCapability ImageCubeArray\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, ReadSuccess4) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_spd_0002 %uniform_image_f32_spd_0002
%res1 = OpImageRead %f32vec4 %img %u32vec2_01
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, ReadNeedCapabilityStorageImageReadWithoutFormat) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0000 %uniform_image_u32_2d_0000
%res1 = OpImageRead %u32vec4 %img %u32vec2_01
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Capability StorageImageReadWithoutFormat is required "
                        "to read storage "
                        "image: ImageRead"));
}

TEST_F(ValidateImage, ReadNeedCapabilityImage1D) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_1d_0002_rgba32f %uniform_image_f32_1d_0002_rgba32f
%res1 = OpImageRead %f32vec4 %img %u32vec2_01
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Capability Image1D is required to access storage image: ImageRead"));
}

TEST_F(ValidateImage, ReadNeedCapabilityImageCubeArray) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0102_rgba32f %uniform_image_f32_cube_0102_rgba32f
%res1 = OpImageRead %f32vec4 %img %u32vec3_012
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Capability ImageCubeArray is required to access storage image: "
          "ImageRead"));
}

// TODO(atgoo@github.com) Disabled until the spec is clarified.
TEST_F(ValidateImage, DISABLED_ReadWrongResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0000 %uniform_image_u32_2d_0000
%res1 = OpImageRead %f32 %img %u32vec2_01
)";

  const std::string extra = "\nOpCapability StorageImageReadWithoutFormat\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Expected Result Type to be int or float vector type: ImageRead"));
}

// TODO(atgoo@github.com) Disabled until the spec is clarified.
TEST_F(ValidateImage, DISABLED_ReadWrongNumComponentsResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0000 %uniform_image_u32_2d_0000
%res1 = OpImageRead %f32vec3 %img %u32vec2_01
)";

  const std::string extra = "\nOpCapability StorageImageReadWithoutFormat\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Result Type to have 4 components: ImageRead"));
}

TEST_F(ValidateImage, ReadNotImage) {
  const std::string body = R"(
%sampler = OpLoad %type_sampler %uniform_sampler
%res1 = OpImageRead %f32vec4 %sampler %u32vec2_01
)";

  const std::string extra = "\nOpCapability StorageImageReadWithoutFormat\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Image to be of type OpTypeImage: ImageRead"));
}

TEST_F(ValidateImage, ReadImageSampled) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageRead %f32vec4 %img %u32vec2_01
)";

  const std::string extra = "\nOpCapability StorageImageReadWithoutFormat\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image 'Sampled' parameter to be 0 or 2: ImageRead"));
}

TEST_F(ValidateImage, ReadWrongSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0000 %uniform_image_u32_2d_0000
%res1 = OpImageRead %f32vec4 %img %u32vec2_01
)";

  const std::string extra = "\nOpCapability StorageImageReadWithoutFormat\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Image 'Sampled Type' to be the same as "
                        "Result Type components: "
                        "ImageRead"));
}

TEST_F(ValidateImage, ReadVoidSampledType) {
  const std::string body = R"(
%img = OpLoad %type_image_void_2d_0002 %uniform_image_void_2d_0002
%res1 = OpImageRead %f32vec4 %img %u32vec2_01
%res2 = OpImageRead %u32vec4 %img %u32vec2_01
%res3 = OpImageRead %s32vec4 %img %u32vec2_01
)";

  const std::string extra = "\nOpCapability StorageImageReadWithoutFormat\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, ReadWrongCoordinateType) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0000 %uniform_image_u32_2d_0000
%res1 = OpImageRead %u32vec4 %img %f32vec2_00
)";

  const std::string extra = "\nOpCapability StorageImageReadWithoutFormat\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Coordinate to be int scalar or vector: ImageRead"));
}

TEST_F(ValidateImage, ReadCoordinateSizeTooSmall) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0000 %uniform_image_u32_2d_0000
%res1 = OpImageRead %u32vec4 %img %u32_1
)";

  const std::string extra = "\nOpCapability StorageImageReadWithoutFormat\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to have at least 2 components, "
                        "but given only 1: "
                        "ImageRead"));
}

TEST_F(ValidateImage, WriteSuccess1) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0000 %uniform_image_u32_2d_0000
%res1 = OpImageWrite %img %u32vec2_01 %u32vec4_0123
)";

  const std::string extra = "\nOpCapability StorageImageWriteWithoutFormat\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, WriteSuccess2) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_1d_0002_rgba32f %uniform_image_f32_1d_0002_rgba32f
%res1 = OpImageWrite %img %u32_1 %f32vec4_0000
)";

  const std::string extra = "\nOpCapability Image1D\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, WriteSuccess3) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0102_rgba32f %uniform_image_f32_cube_0102_rgba32f
%res1 = OpImageWrite %img %u32vec3_012 %f32vec4_0000
)";

  const std::string extra = "\nOpCapability ImageCubeArray\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, WriteSuccess4) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0010 %uniform_image_f32_2d_0010
;TODO(atgoo@github.com) Is it legal to write to MS image without sample index?
%res1 = OpImageWrite %img %u32vec2_01 %f32vec4_0000
%res2 = OpImageWrite %img %u32vec2_01 %f32vec4_0000 Sample %u32_1
)";

  const std::string extra = "\nOpCapability StorageImageWriteWithoutFormat\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, WriteSubpassData) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_spd_0002 %uniform_image_f32_spd_0002
%res1 = OpImageWrite %img %u32vec2_01 %f32vec4_0000
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Image 'Dim' cannot be SubpassData: ImageWrite"));
}

TEST_F(ValidateImage, WriteNeedCapabilityStorageImageWriteWithoutFormat) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0000 %uniform_image_u32_2d_0000
%res1 = OpImageWrite %img %u32vec2_01 %u32vec4_0123
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Capability StorageImageWriteWithoutFormat is required to write to "
          "storage image: ImageWrite"));
}

TEST_F(ValidateImage, WriteNeedCapabilityImage1D) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_1d_0002_rgba32f %uniform_image_f32_1d_0002_rgba32f
%res1 = OpImageWrite %img %u32vec2_01 %f32vec4_0000
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Capability Image1D is required to access storage "
                        "image: ImageWrite"));
}

TEST_F(ValidateImage, WriteNeedCapabilityImageCubeArray) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_cube_0102_rgba32f %uniform_image_f32_cube_0102_rgba32f
%res1 = OpImageWrite %img %u32vec3_012 %f32vec4_0000
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Capability ImageCubeArray is required to access storage image: "
          "ImageWrite"));
}

TEST_F(ValidateImage, WriteNotImage) {
  const std::string body = R"(
%sampler = OpLoad %type_sampler %uniform_sampler
%res1 = OpImageWrite %sampler %u32vec2_01 %f32vec4_0000
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image to be of type OpTypeImage: ImageWrite"));
}

TEST_F(ValidateImage, WriteImageSampled) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageWrite %img %u32vec2_01 %f32vec4_0000
)";

  const std::string extra = "\nOpCapability StorageImageWriteWithoutFormat\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image 'Sampled' parameter to be 0 or 2: ImageWrite"));
}

TEST_F(ValidateImage, WriteWrongCoordinateType) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0000 %uniform_image_u32_2d_0000
%res1 = OpImageWrite %img %f32vec2_00 %u32vec4_0123
)";

  const std::string extra = "\nOpCapability StorageImageWriteWithoutFormat\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Coordinate to be int scalar or vector: ImageWrite"));
}

TEST_F(ValidateImage, WriteCoordinateSizeTooSmall) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0000 %uniform_image_u32_2d_0000
%res1 = OpImageWrite %img %u32_1 %u32vec4_0123
)";

  const std::string extra = "\nOpCapability StorageImageWriteWithoutFormat\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to have at least 2 components, "
                        "but given only 1: "
                        "ImageWrite"));
}

TEST_F(ValidateImage, WriteTexelWrongType) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0000 %uniform_image_u32_2d_0000
%res1 = OpImageWrite %img %u32vec2_01 %img
)";

  const std::string extra = "\nOpCapability StorageImageWriteWithoutFormat\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Expected Texel to be int or float vector or scalar: ImageWrite"));
}

TEST_F(ValidateImage, DISABLED_WriteTexelNotVector4) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0000 %uniform_image_u32_2d_0000
%res1 = OpImageWrite %img %u32vec2_01 %u32vec3_012
)";

  const std::string extra = "\nOpCapability StorageImageWriteWithoutFormat\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Texel to have 4 components: ImageWrite"));
}

TEST_F(ValidateImage, WriteTexelWrongComponentType) {
  const std::string body = R"(
%img = OpLoad %type_image_u32_2d_0000 %uniform_image_u32_2d_0000
%res1 = OpImageWrite %img %u32vec2_01 %f32vec4_0000
)";

  const std::string extra = "\nOpCapability StorageImageWriteWithoutFormat\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Expected Image 'Sampled Type' to be the same as Texel components: "
          "ImageWrite"));
}

TEST_F(ValidateImage, WriteSampleNotInteger) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0010 %uniform_image_f32_2d_0010
%res1 = OpImageWrite %img %u32vec2_01 %f32vec4_0000 Sample %f32_1
)";

  const std::string extra = "\nOpCapability StorageImageWriteWithoutFormat\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Image Operand Sample to be int scalar: "
                        "ImageWrite"));
}

TEST_F(ValidateImage, SampleNotMultisampled) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0002 %uniform_image_f32_2d_0002
%res2 = OpImageWrite %img %u32vec2_01 %f32vec4_0000 Sample %u32_1
)";

  const std::string extra = "\nOpCapability StorageImageWriteWithoutFormat\n";
  CompileSuccessfully(GenerateShaderCode(body, extra).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Image Operand Sample requires non-zero 'MS' parameter: ImageWrite"));
}

TEST_F(ValidateImage, SampleWrongOpcode) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0010 %uniform_image_f32_2d_0010
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0010 %img %sampler
%res1 = OpImageSampleExplicitLod %f32vec4 %simg %f32vec2_00 Sample %u32_1
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Image Operand Sample can only be used with "
                        "OpImageFetch, OpImageRead "
                        "and OpImageWrite: ImageSampleExplicitLod"));
}

TEST_F(ValidateImage, SampleImageToImageSuccess) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%img2 = OpImage %type_image_f32_2d_0001 %simg
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, SampleImageToImageWrongResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%img2 = OpImage %type_sampled_image_f32_2d_0001 %simg
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Result Type to be OpTypeImage: Image"));
}

TEST_F(ValidateImage, SampleImageToImageNotSampledImage) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%img2 = OpImage %type_image_f32_2d_0001 %img
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Expected Sample Image to be of type OpTypeSampleImage: Image"));
}

TEST_F(ValidateImage, SampleImageToImageNotTheSameImageType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%img2 = OpImage %type_image_f32_2d_0002 %simg
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Sample Image image type to be equal to "
                        "Result Type: Image"));
}

TEST_F(ValidateImage, QueryFormatSuccess) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageQueryFormat %u32 %img
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, QueryFormatWrongResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageQueryFormat %bool %img
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Expected Result Type to be int scalar type: ImageQueryFormat"));
}

TEST_F(ValidateImage, QueryFormatNotImage) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageQueryFormat %u32 %simg
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Expected operand to be of type OpTypeImage: ImageQueryFormat"));
}

TEST_F(ValidateImage, QueryOrderSuccess) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageQueryOrder %u32 %img
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, QueryOrderWrongResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageQueryOrder %bool %img
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Result Type to be int scalar type: ImageQueryOrder"));
}

TEST_F(ValidateImage, QueryOrderNotImage) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageQueryOrder %u32 %simg
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected operand to be of type OpTypeImage: ImageQueryOrder"));
}

TEST_F(ValidateImage, QuerySizeLodSuccess) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageQuerySizeLod %u32vec2 %img %u32_1
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, QuerySizeLodWrongResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageQuerySizeLod %f32vec2 %img %u32_1
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Result Type to be int scalar or vector type: "
                        "ImageQuerySizeLod"));
}

TEST_F(ValidateImage, QuerySizeLodResultTypeWrongSize) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageQuerySizeLod %u32 %img %u32_1
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Result Type has 1 components, but 2 expected: ImageQuerySizeLod"));
}

TEST_F(ValidateImage, QuerySizeLodNotImage) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageQuerySizeLod %u32vec2 %simg %u32_1
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image to be of type OpTypeImage: ImageQuerySizeLod"));
}

TEST_F(ValidateImage, QuerySizeLodWrongImageDim) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_rect_0001 %uniform_image_f32_rect_0001
%res1 = OpImageQuerySizeLod %u32vec2 %img %u32_1
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Image 'Dim' must be 1D, 2D, 3D or Cube: ImageQuerySizeLod"));
}

TEST_F(ValidateImage, QuerySizeLodMultisampled) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0010 %uniform_image_f32_2d_0010
%res1 = OpImageQuerySizeLod %u32vec2 %img %u32_1
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Image 'MS' must be 0: ImageQuerySizeLod"));
}

TEST_F(ValidateImage, QuerySizeLodWrongLodType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageQuerySizeLod %u32vec2 %img %u32vec2_01
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Level of Detail to be int or float scalar: "
                        "ImageQuerySizeLod"));
}

TEST_F(ValidateImage, QuerySizeSuccess) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageQuerySize %u32vec2 %img
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, QuerySizeWrongResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageQuerySize %f32vec2 %img
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Result Type to be int scalar or vector type: "
                        "ImageQuerySize"));
}

TEST_F(ValidateImage, QuerySizeNotImage) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageQuerySize %u32vec2 %simg
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image to be of type OpTypeImage: ImageQuerySize"));
}

// TODO(atgoo@github.com) Add more tests for OpQuerySize.

TEST_F(ValidateImage, QueryLodSuccessKernel) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageQueryLod %f32vec2 %simg %f32vec2_hh
%res2 = OpImageQueryLod %f32vec2 %simg %u32vec2_01
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, QueryLodSuccessShader) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageQueryLod %f32vec2 %simg %f32vec2_hh
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, QueryLodWrongResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageQueryLod %u32vec2 %simg %f32vec2_hh
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Result Type to be float vector type: ImageQueryLod"));
}

TEST_F(ValidateImage, QueryLodResultTypeWrongSize) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageQueryLod %f32vec3 %simg %f32vec2_hh
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Result Type to have 2 components: ImageQueryLod"));
}

TEST_F(ValidateImage, QueryLodNotSampledImage) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageQueryLod %f32vec2 %img %f32vec2_hh
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image operand to be of type OpTypeSampledImage: "
                "ImageQueryLod"));
}

TEST_F(ValidateImage, QueryLodWrongDim) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_rect_0001 %uniform_image_f32_rect_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_rect_0001 %img %sampler
%res1 = OpImageQueryLod %f32vec2 %simg %f32vec2_hh
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Image 'Dim' must be 1D, 2D, 3D or Cube: ImageQueryLod"));
}

TEST_F(ValidateImage, QueryLodWrongCoordinateType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageQueryLod %f32vec2 %simg %u32vec2_01
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Expected Coordinate to be float scalar or vector: ImageQueryLod"));
}

TEST_F(ValidateImage, QueryLodCoordinateSizeTooSmall) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageQueryLod %f32vec2 %simg %f32_0
)";

  CompileSuccessfully(GenerateShaderCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Expected Coordinate to have at least 2 components, "
                        "but given only 1: "
                        "ImageQueryLod"));
}

TEST_F(ValidateImage, QueryLevelsSuccess) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageQueryLevels %u32 %img
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, QueryLevelsWrongResultType) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageQueryLevels %f32 %img
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Expected Result Type to be int scalar type: ImageQueryLevels"));
}

TEST_F(ValidateImage, QueryLevelsNotImage) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageQueryLevels %u32 %simg
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Expected Image to be of type OpTypeImage: ImageQueryLevels"));
}

TEST_F(ValidateImage, QueryLevelsWrongDim) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_rect_0001 %uniform_image_f32_rect_0001
%res1 = OpImageQueryLevels %u32 %img
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Image 'Dim' must be 1D, 2D, 3D or Cube: ImageQueryLevels"));
}

TEST_F(ValidateImage, QuerySamplesSuccess) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0010 %uniform_image_f32_2d_0010
%res1 = OpImageQuerySamples %u32 %img
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateImage, QuerySamplesNot2D) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_3d_0010 %uniform_image_f32_3d_0010
%res1 = OpImageQuerySamples %u32 %img
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Image 'Dim' must be 2D: ImageQuerySamples"));
}

TEST_F(ValidateImage, QuerySamplesNotMultisampled) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%res1 = OpImageQuerySamples %u32 %img
)";

  CompileSuccessfully(GenerateKernelCode(body).c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Image 'MS' must be 1: ImageQuerySamples"));
}

TEST_F(ValidateImage, QueryLodWrongExecutionModel) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageQueryLod %f32vec2 %simg %f32vec2_hh
)";

  CompileSuccessfully(GenerateShaderCode(body, "", "Vertex").c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("OpImageQueryLod requires Fragment execution model"));
}

TEST_F(ValidateImage, QueryLodWrongExecutionModelWithFunc) {
  const std::string body = R"(
%call_ret = OpFunctionCall %void %my_func
OpReturn
OpFunctionEnd
%my_func = OpFunction %void None %func
%my_func_entry = OpLabel
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageQueryLod %f32vec2 %simg %f32vec2_hh
)";

  CompileSuccessfully(GenerateShaderCode(body, "", "Vertex").c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("OpImageQueryLod requires Fragment execution model"));
}

TEST_F(ValidateImage, ImplicitLodWrongExecutionModel) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_2d_0001 %uniform_image_f32_2d_0001
%sampler = OpLoad %type_sampler %uniform_sampler
%simg = OpSampledImage %type_sampled_image_f32_2d_0001 %img %sampler
%res1 = OpImageSampleImplicitLod %f32vec4 %simg %f32vec2_hh
)";

  CompileSuccessfully(GenerateShaderCode(body, "", "Vertex").c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("ImplicitLod instructions require Fragment execution model"));
}

TEST_F(ValidateImage, ReadSubpassDataWrongExecutionModel) {
  const std::string body = R"(
%img = OpLoad %type_image_f32_spd_0002 %uniform_image_f32_spd_0002
%res1 = OpImageRead %f32vec4 %img %u32vec2_01
)";

  const std::string extra = "\nOpCapability StorageImageReadWithoutFormat\n";
  CompileSuccessfully(GenerateShaderCode(body, extra, "Vertex").c_str());
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(), HasSubstr(
      "Dim SubpassData requires Fragment execution model: ImageRead"));
}

}  // anonymous namespace
