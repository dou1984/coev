#include "consumer_message.h"

const std::string &ConsumerMessage::key()
{
    return m_key;
}
const std::string &ConsumerMessage::value()
{
    return m_value;
}
