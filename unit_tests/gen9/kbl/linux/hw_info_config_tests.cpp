/*
 * Copyright (c) 2017, Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "unit_tests/helpers/gtest_helpers.h"
#include "unit_tests/os_interface/linux/hw_info_config_tests.h"

using namespace OCLRT;
using namespace std;

struct HwInfoConfigTestLinuxKbl : HwInfoConfigTestLinux {
    void SetUp() override {
        HwInfoConfigTestLinux::SetUp();
        drm->StoredDeviceID = IKBL_GT2_DT_DEVICE_F0_ID;
        drm->setGtType(GTTYPE_GT2);
    }
};

KBLTEST_F(HwInfoConfigTestLinuxKbl, configureHwInfo) {
    auto hwInfoConfig = HwInfoConfig::get(productFamily);
    int ret = hwInfoConfig->configureHwInfo(pInHwInfo, &outHwInfo, osInterface);
    EXPECT_EQ(0, ret);
    EXPECT_EQ((unsigned short)drm->StoredDeviceID, outHwInfo.pPlatform->usDeviceID);
    EXPECT_EQ((unsigned short)drm->StoredDeviceRevID, outHwInfo.pPlatform->usRevId);
    EXPECT_EQ((uint32_t)drm->StoredEUVal, outHwInfo.pSysInfo->EUCount);
    EXPECT_EQ((uint32_t)drm->StoredSSVal, outHwInfo.pSysInfo->SubSliceCount);

    EXPECT_EQ(GTTYPE_GT2, outHwInfo.pPlatform->eGTType);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGT1);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGT1_5);
    EXPECT_EQ(1u, outHwInfo.pSkuTable->ftrGT2);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGT3);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGT4);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGTA);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGTC);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGTX);

    ReleaseOutHwInfoStructs();

    drm->StoredDeviceID = IKBL_GT1_ULT_DEVICE_F0_ID;
    drm->StoredSSVal = 3;
    drm->setGtType(GTTYPE_GT1);
    ret = hwInfoConfig->configureHwInfo(pInHwInfo, &outHwInfo, osInterface);
    EXPECT_EQ(0, ret);
    EXPECT_EQ((unsigned short)drm->StoredDeviceID, outHwInfo.pPlatform->usDeviceID);
    EXPECT_EQ((unsigned short)drm->StoredDeviceRevID, outHwInfo.pPlatform->usRevId);
    EXPECT_EQ((uint32_t)drm->StoredEUVal, outHwInfo.pSysInfo->EUCount);
    EXPECT_EQ((uint32_t)drm->StoredSSVal, outHwInfo.pSysInfo->SubSliceCount);
    EXPECT_EQ(1u, outHwInfo.pSysInfo->SliceCount);

    EXPECT_EQ(GTTYPE_GT1, outHwInfo.pPlatform->eGTType);
    EXPECT_EQ(1u, outHwInfo.pSkuTable->ftrGT1);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGT1_5);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGT2);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGT3);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGT4);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGTA);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGTC);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGTX);

    ReleaseOutHwInfoStructs();

    drm->StoredDeviceID = IKBL_GT1_5_ULX_DEVICE_F0_ID;
    drm->setGtType(GTTYPE_GT1_5);
    ret = hwInfoConfig->configureHwInfo(pInHwInfo, &outHwInfo, osInterface);
    EXPECT_EQ(0, ret);
    EXPECT_EQ((unsigned short)drm->StoredDeviceID, outHwInfo.pPlatform->usDeviceID);
    EXPECT_EQ((unsigned short)drm->StoredDeviceRevID, outHwInfo.pPlatform->usRevId);
    EXPECT_EQ((uint32_t)drm->StoredEUVal, outHwInfo.pSysInfo->EUCount);
    EXPECT_EQ((uint32_t)drm->StoredSSVal, outHwInfo.pSysInfo->SubSliceCount);

    EXPECT_EQ(GTTYPE_GT1_5, outHwInfo.pPlatform->eGTType);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGT1);
    EXPECT_EQ(1u, outHwInfo.pSkuTable->ftrGT1_5);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGT2);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGT3);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGT4);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGTA);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGTC);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGTX);

    ReleaseOutHwInfoStructs();

    drm->StoredDeviceID = IKBL_GT3_ULT_DEVICE_F0_ID;
    drm->StoredSSVal = 6;
    drm->setGtType(GTTYPE_GT3);
    ret = hwInfoConfig->configureHwInfo(pInHwInfo, &outHwInfo, osInterface);
    EXPECT_EQ(0, ret);
    EXPECT_EQ((unsigned short)drm->StoredDeviceID, outHwInfo.pPlatform->usDeviceID);
    EXPECT_EQ((unsigned short)drm->StoredDeviceRevID, outHwInfo.pPlatform->usRevId);
    EXPECT_EQ((uint32_t)drm->StoredEUVal, outHwInfo.pSysInfo->EUCount);
    EXPECT_EQ((uint32_t)drm->StoredSSVal, outHwInfo.pSysInfo->SubSliceCount);
    EXPECT_EQ(2u, outHwInfo.pSysInfo->SliceCount);

    EXPECT_EQ(GTTYPE_GT3, outHwInfo.pPlatform->eGTType);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGT1);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGT1_5);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGT2);
    EXPECT_EQ(1u, outHwInfo.pSkuTable->ftrGT3);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGT4);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGTA);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGTC);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGTX);

    ReleaseOutHwInfoStructs();

    drm->StoredDeviceID = IKBL_GT4_HALO_DEVICE_F0_ID;
    drm->StoredSSVal = 6;
    drm->setGtType(GTTYPE_GT4);
    ret = hwInfoConfig->configureHwInfo(pInHwInfo, &outHwInfo, osInterface);
    EXPECT_EQ(0, ret);
    EXPECT_EQ((unsigned short)drm->StoredDeviceID, outHwInfo.pPlatform->usDeviceID);
    EXPECT_EQ((unsigned short)drm->StoredDeviceRevID, outHwInfo.pPlatform->usRevId);
    EXPECT_EQ((uint32_t)drm->StoredEUVal, outHwInfo.pSysInfo->EUCount);
    EXPECT_EQ((uint32_t)drm->StoredSSVal, outHwInfo.pSysInfo->SubSliceCount);
    EXPECT_EQ(2u, outHwInfo.pSysInfo->SliceCount);

    EXPECT_EQ(GTTYPE_GT4, outHwInfo.pPlatform->eGTType);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGT1);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGT1_5);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGT2);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGT3);
    EXPECT_EQ(1u, outHwInfo.pSkuTable->ftrGT4);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGTA);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGTC);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrGTX);
}

KBLTEST_F(HwInfoConfigTestLinuxKbl, negativeUnknownDevId) {
    drm->StoredDeviceID = 0;
    auto hwInfoConfig = HwInfoConfig::get(productFamily);
    int ret = hwInfoConfig->configureHwInfo(pInHwInfo, &outHwInfo, osInterface);
    EXPECT_EQ(-1, ret);
}

KBLTEST_F(HwInfoConfigTestLinuxKbl, negativeFailedIoctlDevId) {
    drm->StoredRetValForDeviceID = -2;
    auto hwInfoConfig = HwInfoConfig::get(productFamily);
    int ret = hwInfoConfig->configureHwInfo(pInHwInfo, &outHwInfo, osInterface);
    EXPECT_EQ(-2, ret);
}

KBLTEST_F(HwInfoConfigTestLinuxKbl, negativeFailedIoctlDevRevId) {
    drm->StoredRetValForDeviceRevID = -3;
    auto hwInfoConfig = HwInfoConfig::get(productFamily);
    int ret = hwInfoConfig->configureHwInfo(pInHwInfo, &outHwInfo, osInterface);
    EXPECT_EQ(-3, ret);
}

KBLTEST_F(HwInfoConfigTestLinuxKbl, negativeFailedIoctlEuCount) {
    drm->StoredRetValForEUVal = -4;
    auto hwInfoConfig = HwInfoConfig::get(productFamily);
    int ret = hwInfoConfig->configureHwInfo(pInHwInfo, &outHwInfo, osInterface);
    EXPECT_EQ(-4, ret);
}

KBLTEST_F(HwInfoConfigTestLinuxKbl, negativeFailedIoctlSsCount) {
    drm->StoredRetValForSSVal = -5;
    auto hwInfoConfig = HwInfoConfig::get(productFamily);
    int ret = hwInfoConfig->configureHwInfo(pInHwInfo, &outHwInfo, osInterface);
    EXPECT_EQ(-5, ret);
}

KBLTEST_F(HwInfoConfigTestLinuxKbl, configureHwInfoWaFlags) {
    auto hwInfoConfig = HwInfoConfig::get(productFamily);

    drm->StoredDeviceRevID = 0;
    int ret = hwInfoConfig->configureHwInfo(pInHwInfo, &outHwInfo, osInterface);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(1u, outHwInfo.pWaTable->waDisableLSQCROPERFforOCL);
    EXPECT_EQ(1u, outHwInfo.pWaTable->waEncryptedEdramOnlyPartials);
    EXPECT_EQ(1u, outHwInfo.pWaTable->waForcePcBbFullCfgRestore);

    ReleaseOutHwInfoStructs();

    drm->StoredDeviceRevID = 7;
    ret = hwInfoConfig->configureHwInfo(pInHwInfo, &outHwInfo, osInterface);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0u, outHwInfo.pWaTable->waDisableLSQCROPERFforOCL);
    EXPECT_EQ(0u, outHwInfo.pWaTable->waEncryptedEdramOnlyPartials);
    EXPECT_EQ(1u, outHwInfo.pWaTable->waForcePcBbFullCfgRestore);

    ReleaseOutHwInfoStructs();

    drm->StoredDeviceRevID = 9;
    ret = hwInfoConfig->configureHwInfo(pInHwInfo, &outHwInfo, osInterface);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0u, outHwInfo.pWaTable->waDisableLSQCROPERFforOCL);
    EXPECT_EQ(0u, outHwInfo.pWaTable->waEncryptedEdramOnlyPartials);
    EXPECT_EQ(0u, outHwInfo.pWaTable->waForcePcBbFullCfgRestore);
}

KBLTEST_F(HwInfoConfigTestLinuxKbl, configureHwInfoEdram) {
    auto hwInfoConfig = HwInfoConfig::get(productFamily);

    int ret = hwInfoConfig->configureHwInfo(pInHwInfo, &outHwInfo, osInterface);
    EXPECT_EQ(0, ret);
    EXPECT_EQ_VAL(0u, outHwInfo.pSysInfo->EdramSizeInKb);
    EXPECT_EQ(0u, outHwInfo.pSkuTable->ftrEDram);
    ReleaseOutHwInfoStructs();

    drm->StoredDeviceID = IKBL_GT3_28W_ULT_DEVICE_F0_ID;
    drm->setGtType(GTTYPE_GT3);
    ret = hwInfoConfig->configureHwInfo(pInHwInfo, &outHwInfo, osInterface);
    EXPECT_EQ(0, ret);
    EXPECT_EQ_VAL((64u * 1024u), outHwInfo.pSysInfo->EdramSizeInKb);
    EXPECT_EQ(1u, outHwInfo.pSkuTable->ftrEDram);
    ReleaseOutHwInfoStructs();

    drm->StoredDeviceID = IKBL_GT3_15W_ULT_DEVICE_F0_ID;
    ret = hwInfoConfig->configureHwInfo(pInHwInfo, &outHwInfo, osInterface);
    EXPECT_EQ(0, ret);
    EXPECT_EQ_VAL((64u * 1024u), outHwInfo.pSysInfo->EdramSizeInKb);
    EXPECT_EQ(1u, outHwInfo.pSkuTable->ftrEDram);
}

template <typename T>
class KblHwInfoTests : public ::testing::Test {
};
typedef ::testing::Types<KBL_1x2x6, KBL_1x3x6, KBL_1x3x8, KBL_2x3x8, KBL_3x3x8> kblTestTypes;
TYPED_TEST_CASE(KblHwInfoTests, kblTestTypes);
TYPED_TEST(KblHwInfoTests, gtSetupIsCorrect) {
    GT_SYSTEM_INFO gtSystemInfo;
    memset(&gtSystemInfo, 0, sizeof(gtSystemInfo));
    TypeParam::setupGtSystemInfo(&gtSystemInfo);
    EXPECT_GT(gtSystemInfo.EUCount, 0u);
    EXPECT_GT(gtSystemInfo.ThreadCount, 0u);
    EXPECT_GT(gtSystemInfo.SliceCount, 0u);
    EXPECT_GT(gtSystemInfo.SubSliceCount, 0u);
    EXPECT_GT_VAL(gtSystemInfo.L3CacheSizeInKb, 0u);
    EXPECT_EQ(gtSystemInfo.CsrSizeInMb, 8u);
    EXPECT_FALSE(gtSystemInfo.IsDynamicallyPopulated);
}