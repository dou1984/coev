#pragma once

enum IncrementalAlterConfigsOperation : int8_t
{
    IncrementalAlterConfigsOperationSet = 0,
    IncrementalAlterConfigsOperationDelete = 1,
    IncrementalAlterConfigsOperationAppend = 2,
    IncrementalAlterConfigsOperationSubtract = 3,
};
