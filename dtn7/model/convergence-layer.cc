#include "convergence-layer.h"

namespace ns3 {

namespace dtn7 {

NS_OBJECT_ENSURE_REGISTERED (ConvergenceReceiver);
NS_OBJECT_ENSURE_REGISTERED (ConvergenceSender);
NS_OBJECT_ENSURE_REGISTERED (ConvergenceLayer);

TypeId 
ConvergenceReceiver::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::dtn7::ConvergenceReceiver")
    .SetParent<Object> ()
    .SetGroupName ("Dtn7")
  ;
  return tid;
}

ConvergenceReceiver::ConvergenceReceiver ()
{
}

ConvergenceReceiver::~ConvergenceReceiver ()
{
}

TypeId 
ConvergenceSender::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::dtn7::ConvergenceSender")
    .SetParent<Object> ()
    .SetGroupName ("Dtn7")
  ;
  return tid;
}

ConvergenceSender::ConvergenceSender ()
{
}

ConvergenceSender::~ConvergenceSender ()
{
}

TypeId 
ConvergenceLayer::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::dtn7::ConvergenceLayer")
    .SetParent<Object> ()
    .SetGroupName ("Dtn7")
  ;
  return tid;
}

ConvergenceLayer::ConvergenceLayer ()
{
}

ConvergenceLayer::~ConvergenceLayer ()
{
}

} // namespace dtn7

} // namespace ns3