// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "Mmi.h"
#include "Sample.h"

Sample::Sample(unsigned int maxPayloadSizeBytes)
{
    m_maxPayloadSizeBytes = maxPayloadSizeBytes;
}

int Sample::SetValue(const std::string& value)
{
    m_value = value;
    return MMI_OK;
}

std::string Sample::GetValue()
{
    return m_value;
}

unsigned int Sample::GetMaxPayloadSizeBytes()
{
    return m_maxPayloadSizeBytes;
}