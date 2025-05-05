#include "bundle-store.h"

namespace ns3 {

namespace dtn7 {

NS_OBJECT_ENSURE_REGISTERED (BundleStore);

TypeId 
BundleStore::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::dtn7::BundleStore")
    .SetParent<Object> ()
    .SetGroupName ("Dtn7")
  ;
  return tid;
}

BundleStore::BundleStore ()
{
}

BundleStore::~BundleStore ()
{
}

} // namespace dtn7

} // namespace ns3