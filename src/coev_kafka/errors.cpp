#include "errors.h"
#include <coev/log.h>
#include <sstream>

const char *KErrorToString(KError err)
{
    switch (err)
    {
    case KError::ErrNoError:
        return "kafka server: Not an error, why are you printing me?";
    case KError::ErrUnknown:
        return "kafka server: Unexpected (unknown?) server error";
    case KError::ErrOffsetOutOfRange:
        return "kafka server: The requested offset is outside the range of offsets maintained by the server for the given topic/partition";
    case KError::ErrInvalidMessage:
        return "kafka server: Message contents does not match its CRC";
    case KError::ErrUnknownTopicOrPartition:
        return "kafka server: Request was for a topic or partition that does not exist on this broker";
    case KError::ErrInvalidMessageSize:
        return "kafka server: The message has a negative size";
    case KError::ErrLeaderNotAvailable:
        return "kafka server: In the middle of a leadership election, there is currently no leader for this partition and hence it is unavailable for writes";
    case KError::ErrNotLeaderForPartition:
        return "kafka server: Tried to send a message to a replica that is not the leader for some partition. Your metadata is out of date";
    case KError::ErrRequestTimedOut:
        return "kafka server: Request exceeded the user-specified time limit in the request";
    case KError::ErrBrokerNotAvailable:
        return "kafka server: Broker not available. Not a client facing error, we should never receive this!!!";
    case KError::ErrReplicaNotAvailable:
        return "kafka server: Replica information not available, one or more brokers are down";
    case KError::ErrMessageSizeTooLarge:
        return "kafka server: Message was too large, server rejected it to avoid allocation error";
    case KError::ErrStaleControllerEpochCode:
        return "kafka server: StaleControllerEpochCode (internal error code for broker-to-broker communication)";
    case KError::ErrOffsetMetadataTooLarge:
        return "kafka server: Specified a string larger than the configured maximum for offset metadata";
    case KError::ErrNetworkException:
        return "kafka server: The server disconnected before a response was received";
    case KError::ErrOffsetsLoadInProgress:
        return "kafka server: The coordinator is still loading offsets and cannot currently process requests";
    case KError::ErrConsumerCoordinatorNotAvailable:
        return "kafka server: The coordinator is not available";
    case KError::ErrNotCoordinatorForConsumer:
        return "kafka server: Request was for a consumer group that is not coordinated by this broker";
    case KError::ErrInvalidTopic:
        return "kafka server: The request attempted to perform an operation on an invalid topic";
    case KError::ErrMessageSetSizeTooLarge:
        return "kafka server: The request included message batch larger than the configured segment size on the server";
    case KError::ErrNotEnoughReplicas:
        return "kafka server: Messages are rejected since there are fewer in-sync replicas than required";
    case KError::ErrNotEnoughReplicasAfterAppend:
        return "kafka server: Messages are written to the log, but to fewer in-sync replicas than required";
    case KError::ErrInvalidRequiredAcks:
        return "kafka server: The number of required acks is invalid (should be either -1, 0, or 1)";
    case KError::ErrIllegalGeneration:
        return "kafka server: The provided generation id is not the current generation";
    case KError::ErrInconsistentGroupProtocol:
        return "kafka server: The provider group protocol type is incompatible with the other members";
    case KError::ErrInvalidGroupId:
        return "kafka server: The provided group id was empty";
    case KError::ErrUnknownMemberId:
        return "kafka server: The provided member is not known in the current generation";
    case KError::ErrInvalidSessionTimeout:
        return "kafka server: The provided session timeout is outside the allowed range";
    case KError::ErrRebalanceInProgress:
        return "kafka server: A rebalance for the group is in progress. Please re-join the group";
    case KError::ErrInvalidCommitOffsetSize:
        return "kafka server: The provided commit metadata was too large";
    case KError::ErrTopicAuthorizationFailed:
        return "kafka server: The client is not authorized to access this topic";
    case KError::ErrGroupAuthorizationFailed:
        return "kafka server: The client is not authorized to access this group";
    case KError::ErrClusterAuthorizationFailed:
        return "kafka server: The client is not authorized to send this request type";
    case KError::ErrInvalidTimestamp:
        return "kafka server: The timestamp of the message is out of acceptable range";
    case KError::ErrUnsupportedSASLMechanism:
        return "kafka server: The broker does not support the requested SASL mechanism";
    case KError::ErrIllegalSASLState:
        return "kafka server: Request is not valid given the current SASL state";
    case KError::ErrUnsupportedVersion:
        return "kafka server: The version of API is not supported";
    case KError::ErrTopicAlreadyExists:
        return "kafka server: Topic with this name already exists";
    case KError::ErrInvalidPartitions:
        return "kafka server: Number of partitions is invalid";
    case KError::ErrInvalidReplicationFactor:
        return "kafka server: Replication-factor is invalid";
    case KError::ErrInvalidReplicaAssignment:
        return "kafka server: Replica assignment is invalid";
    case KError::ErrInvalidConfig:
        return "kafka server: Configuration is invalid";
    case KError::ErrNotController:
        return "kafka server: This is not the correct controller for this cluster";
    case KError::ErrInvalidRequest:
        return "kafka server: This most likely occurs because of a request being malformed by the client library or the message was sent to an incompatible broker. See the broker logs for more details";
    case KError::ErrUnsupportedForMessageFormat:
        return "kafka server: The requested operation is not supported by the message format version";
    case KError::ErrPolicyViolation:
        return "kafka server: Request parameters do not satisfy the configured policy";
    case KError::ErrOutOfOrderSequenceNumber:
        return "kafka server: The broker received an out of order sequence number";
    case KError::ErrDuplicateSequenceNumber:
        return "kafka server: The broker received a duplicate sequence number";
    case KError::ErrInvalidProducerEpoch:
        return "kafka server: Producer attempted an operation with an old epoch";
    case KError::ErrInvalidTxnState:
        return "kafka server: The producer attempted a transactional operation in an invalid state";
    case KError::ErrInvalidProducerIDMapping:
        return "kafka server: The producer attempted to use a producer id which is not currently assigned to its transactional id";
    case KError::ErrInvalidTransactionTimeout:
        return "kafka server: The transaction timeout is larger than the maximum value allowed by the broker (as configured by max.transaction.timeout.ms)";
    case KError::ErrConcurrentTransactions:
        return "kafka server: The producer attempted to update a transaction while another concurrent operation on the same transaction was ongoing";
    case KError::ErrTransactionCoordinatorFenced:
        return "kafka server: The transaction coordinator sending a WriteTxnMarker is no longer the current coordinator for a given producer";
    case KError::ErrTransactionalIDAuthorizationFailed:
        return "kafka server: Transactional ID authorization failed";
    case KError::ErrSecurityDisabled:
        return "kafka server: Security features are disabled";
    case KError::ErrOperationNotAttempted:
        return "kafka server: The broker did not attempt to execute this operation";
    case KError::ErrKafkaStorageError:
        return "kafka server: Disk error when trying to access log file on the disk";
    case KError::ErrLogDirNotFound:
        return "kafka server: The specified log directory is not found in the broker config";
    case KError::ErrSASLAuthenticationFailed:
        return "kafka server: SASL Authentication failed";
    case KError::ErrUnknownProducerID:
        return "kafka server: The broker could not locate the producer metadata associated with the Producer ID";
    case KError::ErrReassignmentInProgress:
        return "kafka server: A partition reassignment is in progress";
    case KError::ErrDelegationTokenAuthDisabled:
        return "kafka server: Delegation Token feature is not enabled";
    case KError::ErrDelegationTokenNotFound:
        return "kafka server: Delegation Token is not found on server";
    case KError::ErrDelegationTokenOwnerMismatch:
        return "kafka server: Specified Principal is not valid Owner/Renewer";
    case KError::ErrDelegationTokenRequestNotAllowed:
        return "kafka server: Delegation Token requests are not allowed on PLAINTEXT/1-way SSL channels and on delegation token authenticated channels";
    case KError::ErrDelegationTokenAuthorizationFailed:
        return "kafka server: Delegation Token authorization failed";
    case KError::ErrDelegationTokenExpired:
        return "kafka server: Delegation Token is expired";
    case KError::ErrInvalidPrincipalType:
        return "kafka server: Supplied principalType is not supported";
    case KError::ErrNonEmptyGroup:
        return "kafka server: The group is not empty";
    case KError::ErrGroupIDNotFound:
        return "kafka server: The group id does not exist";
    case KError::ErrFetchSessionIDNotFound:
        return "kafka server: The fetch session ID was not found";
    case KError::ErrInvalidFetchSessionEpoch:
        return "kafka server: The fetch session epoch is invalid";
    case KError::ErrListenerNotFound:
        return "kafka server: There is no listener on the leader broker that matches the listener on which metadata request was processed";
    case KError::ErrTopicDeletionDisabled:
        return "kafka server: Topic deletion is disabled";
    case KError::ErrFencedLeaderEpoch:
        return "kafka server: The leader epoch in the request is older than the epoch on the broker";
    case KError::ErrUnknownLeaderEpoch:
        return "kafka server: The leader epoch in the request is newer than the epoch on the broker";
    case KError::ErrUnsupportedCompressionType:
        return "kafka server: The requesting client does not support the compression type of given partition";
    case KError::ErrStaleBrokerEpoch:
        return "kafka server: Broker epoch has changed";
    case KError::ErrOffsetNotAvailable:
        return "kafka server: The leader high watermark has not caught up from a recent leader election so the offsets cannot be guaranteed to be monotonically increasing";
    case KError::ErrMemberIdRequired:
        return "kafka server: The group member needs to have a valid member id before actually entering a consumer group";
    case KError::ErrPreferredLeaderNotAvailable:
        return "kafka server: The preferred leader was not available";
    case KError::ErrGroupMaxSizeReached:
        return "kafka server: Consumer group The consumer group has reached its max size. already has the configured maximum number of members";
    case KError::ErrFencedInstancedId:
        return "kafka server: The broker rejected this static consumer since another consumer with the same group.instance.id has registered with a different member.id";
    case KError::ErrEligibleLeadersNotAvailable:
        return "kafka server: Eligible topic partition leaders are not available";
    case KError::ErrElectionNotNeeded:
        return "kafka server: Leader election not needed for topic partition";
    case KError::ErrNoReassignmentInProgress:
        return "kafka server: No partition reassignment is in progress";
    case KError::ErrGroupSubscribedToTopic:
        return "kafka server: Deleting offsets of a topic is forbidden while the consumer group is actively subscribed to it";
    case KError::ErrInvalidRecord:
        return "kafka server: This record has failed the validation on broker and hence will be rejected";
    case KError::ErrUnstableOffsetCommit:
        return "kafka server: There are unstable offsets that need to be cleared";

    case ErrIOEOF:
        return "kafka: unexpected end of file";
    case ErrTimedOut:
        return "kafka: timed out";
    case ErrOutOfBrokers:
        return "kafka: client has run out of available brokers to talk to";
    case ErrBrokerNotFound:
        return "kafka: broker for ID is not found";
    case ErrInvalidBrokerId:
        return "kafka: broker ID is invalid";
    case ErrClosedClient:
        return "kafka: tried to use a client that was closed";
    case ErrIncompleteResponse:
        return "kafka: response did not contain all the expected topic/partition blocks";
    case ErrInvalidPartition:
        return "kafka: partitioner returned an invalid partition index";
    case ErrAlreadyConnected:
        return "kafka: broker connection already initiated";
    case ErrNotConnected:
        return "kafka: broker not connected";
    case ErrInsufficientData:
        return "kafka: insufficient data to decode packet, more bytes expected";
    case ErrShuttingDown:
        return "kafka: message received by producer in process of shutting down";
    case ErrMessageTooLarge:
        return "kafka: message is larger than Consumer.Fetch.Max";
    case ErrConsumerOffsetNotAdvanced:
        return "kafka: consumer offset was not advanced after a RecordBatch";
    case ErrConsumerOffsetUnsupportedVersion:
        return "kafka: consumer offset is not supported by the broker";
    case ErrControllerNotAvailable:
        return "kafka: controller is not available";
    case ErrNoTopicsToUpdateMetadata:
        return "kafka: no specific topics to update metadata";
    case ErrUnknownScramMechanism:
        return "kafka: unknown SCRAM mechanism provided";
    case ErrReassignPartitions:
        return "kafka: failed to reassign partitions for topic";
    case ErrDeleteRecords:
        return "kafka server: failed to delete records";
    case ErrCreateACLs:
        return "kafka server: failed to create one or more ACL rules";
    case ErrAddPartitionsToTxn:
        return "transaction manager: failed to send partitions to transaction";
    case ErrTxnOffsetCommit:
        return "transaction manager: failed to send offsets to transaction";
    case ErrTransactionNotReady:
        return "transaction manager: transaction is not ready";
    case ErrNonTransactedProducer:
        return "transaction manager: you need to add TransactionalID to producer";
    case ErrTransitionNotAllowed:
        return "transaction manager: invalid transition attempted";
    case ErrCannotTransitionNilError:
        return "transaction manager: cannot transition with a nil error";
    case ErrTxnUnableToParseResponse:
        return "transaction manager: unable to parse response";
    case ErrTopicUnkonw:
        return "kafka: topic is unknown";
    case ErrTopicNoDetails:
        return "kafka: topic has no details";
    case ErrProducerRetryBufferOverflow:
        return "kafka: producer retry buffer overflow";
    case ErrUnsupportedRecordBatchVersion:
        return "kafka server: unsupported record batch version";
    case ErrVariantOverflow:
        return "kafka: variant overflow";
    case ErrTaggedFieldsInNonFlexibleContext:
        return "kafka: tagged fields are not allowed in non-flexible context";
    case ErrInvalidStringLength:
        return "kafka: string length is invalid";
    case ErrUVariantOverflow:
        return "kafka: uvariant overflow";
    case ErrInvalidArrayLength:
        return "kafka: array length is invalid";
    case ErrInvalidBool:
        return "kafka: bool is invalid";
    case ErrValidByteSliceLength:
        return "kafka: byte slice length is invalid";
    case ErrNoConfiguredBrokers:
        return "kafka:You must provide at least one broker address";
    case ErrHeaderVersionIsOld:
        return "kafka:Producing headers requires Kafka at least v0.11";
    case ErrPacketDecodingError:
        return "kafka: packet decoding error";
    case ErrInvalidClientID:
        return "ClientID value is not valid for Kafka versions before 1.0.0";
    case ErrEncodeError:
        return "kafka: encode error";
    case ErrDecodeError:
        return "kafka: decode error";
    case ErrUnknownRecordsType:
        return "kafka: unknown records type";
    case ErrUnknownMessage:
        return "kafka: unknown protocol message key";
    case ErrClosedConsumerGroup:
        return "kafka: consumer group is closed";
    case ErrorGroupClosed:
        return "kafka: consumer group is closed";
    case ErrTopicsProvided:
        return "kafka: topics are provided";
    case ErrPacketEncoding:
        return "kafka: packet encoding error";
    case ErrTopicPartitionConsumed:
        return "kafka: topic partition is consumed";
    case ErrPermitForbidden:
        return "kafka: permit forbidden";
    case ErrMaxMessageBytes:
        return "kafka: Attempt to produce message larger than configured Producer.MaxMessageBytes";
    case ErrThrottled:
        return "kafka: Throttled";
    case ErrAlterClientQuotas:
        return "kafka: AlterClientQuotas";
    case ErrDescribeClientQuotas:
        return "kafka: DescribeClientQuotas";
    case ErrClearupError:
        return "kafka: ClearupError";
    case ErrInvalidCRCTYPE:
        return "kafka: InvalidCRCTYPE";
    case ErrCRCMismatch:
        return "kafka: CRCMismatch";
    case ErrInvalidConsumer:
        return "kafka: InvalidConsumer";
    case ErrSyncProducerError:
        return "kafka: SyncProducerError";
    case ErrSyncProducerSuccess:
        return "kafka: SyncProducerSuccess";
    case ErrStrategyNotFound:
        return "kafka: unable to find selected strategy";
    case ErrCorrelationID:
        return "kafka: mismatch CorrelationID";
    default:
        LOG_ERR("Unknown error %d", err);
        return "Unknown error";
    }
}

bool IsKError(KError err)
{
    return err < ErrLittleEnd;
}
