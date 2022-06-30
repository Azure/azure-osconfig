// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using System;

namespace E2eTesting
{
    public abstract class AbstractDataSource
    {
        protected const int DEFAULT_MAX_WAIT_SECONDS = 90;
        protected int _maxWaitTimeSeconds = DEFAULT_MAX_WAIT_SECONDS;
        abstract public void Initialize();

        abstract public bool SetDesired<T>(string componentName, string objectName, T value, int maxWaitSeconds);
        public bool SetDesired<T>(string componentName, string objectName, T value)
        {
            return SetDesired<T>(componentName, objectName, value, _maxWaitTimeSeconds);
        }
        public bool SetDesired<T>(string componentName, T value, int maxWaitSeconds)
        {
            return SetDesired<T>(componentName, null, value, maxWaitSeconds);
        }
        public bool SetDesired<T>(string componentName, T value)
        {
            return SetDesired<T>(componentName, null, value, _maxWaitTimeSeconds);
        }
        
        abstract public T GetReported<T>(string componentName, string objectName, Func<T, bool> condition, int maxWaitSeconds);
        public T GetReported<T>(string componentName, string objectName, Func<T, bool> condition)
        {
            return GetReported<T>(componentName, objectName, condition, _maxWaitTimeSeconds);
        }
        public T GetReported<T>(string componentName, Func<T, bool> condition, int maxWaitSeconds)
        {
            return GetReported<T>(componentName, null, condition, maxWaitSeconds);
        }
        public T GetReported<T>(string componentName, string objectName, int maxWaitSeconds)
        {
            return GetReported<T>(componentName, objectName, (T) => true, maxWaitSeconds);
        }
        public T GetReported<T>(string componentName, Func<T, bool> condition)
        {
            return GetReported<T>(componentName, null, condition, _maxWaitTimeSeconds);
        }
        public T GetReported<T>(string componentName, string objectName)
        {
            return GetReported<T>(componentName, objectName, (T) => true, _maxWaitTimeSeconds);
        }
        public T GetReported<T>(string componentName, int maxWaitSeconds)
        {
            return GetReported<T>(componentName, null, (T) => true, maxWaitSeconds);
        }
        public T GetReported<T>(string componentName)
        {
            return GetReported<T>(componentName, (T) => true, _maxWaitTimeSeconds);
        }
    }
}