/*
 * Copyright (c) 2018, Intel Corporation
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

#include "runtime/helpers/preamble.inl"

namespace OCLRT {

template <>
void PreambleHelper<BDWFamily>::setupPipeControlInFrontOfCommand(void *pCmd, const HardwareInfo *hwInfo, bool isVfeCommand) {
    ((BDWFamily::PIPE_CONTROL *)pCmd)->setDcFlushEnable(true);
}

template <>
void PreambleHelper<BDWFamily>::programThreadArbitration(LinearStream *pCommandStream, uint32_t threadArbitrationPolicy) {
}

template <>
uint32_t PreambleHelper<BDWFamily>::getAdditionalCommandsSize(const Device &device) {
    return 0;
}

template <>
uint32_t PreambleHelper<BDWFamily>::getL3Config(const HardwareInfo &hwInfo, bool useSLM) {
    uint32_t l3Config = 0;

    switch (hwInfo.pPlatform->eProductFamily) {
    case IGFX_BROADWELL:
        l3Config = getL3ConfigHelper<IGFX_BROADWELL>(useSLM);
        break;
    default:
        l3Config = getL3ConfigHelper<IGFX_BROADWELL>(true);
    }
    return l3Config;
}

template <>
void PreambleHelper<BDWFamily>::programPipelineSelect(LinearStream *pCommandStream, bool mediaSamplerRequired) {
    typedef typename BDWFamily::PIPELINE_SELECT PIPELINE_SELECT;
    auto pCmd = (PIPELINE_SELECT *)pCommandStream->getSpace(sizeof(PIPELINE_SELECT));
    *pCmd = PIPELINE_SELECT::sInit();
    pCmd->setMaskBits(pipelineSelectEnablePipelineSelectMaskBits);
    pCmd->setPipelineSelection(PIPELINE_SELECT::PIPELINE_SELECTION_GPGPU);
}

// Explicitly instantiate PreambleHelper for BDW device family
template struct PreambleHelper<BDWFamily>;
} // namespace OCLRT
