#include "memory-bundle-store.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"  // Added for UintegerValue
#include "ns3/object.h"    // Added for type accessors and checkers
#include <sstream>
#include <functional> // For hash function support

namespace ns3 {

namespace dtn7 {

NS_OBJECT_ENSURE_REGISTERED (MemoryBundleStore);

TypeId 
MemoryBundleStore::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::dtn7::MemoryBundleStore")
    .SetParent<BundleStore> ()
    .SetGroupName ("Dtn7")
    .AddConstructor<MemoryBundleStore> ()
    .AddAttribute ("MaxBundles",
                   "Maximum number of bundles to store",
                   UintegerValue (1000),
                   MakeUintegerAccessor (&MemoryBundleStore::m_maxBundles),
                   MakeUintegerChecker<uint32_t> (1))
  ;
  return tid;
}

MemoryBundleStore::MemoryBundleStore ()
  : m_pushCount (0),
    m_getCount (0),
    m_removeCount (0),
    m_creationTime (Simulator::Now ()),
    m_maxBundles (1000)  // Added to initialize the new member variable
{
}

MemoryBundleStore::~MemoryBundleStore ()
{
}

bool 
MemoryBundleStore::Push (Ptr<Bundle> bundle)
{
  if (!bundle)
    {
      return false;
    }
  
  BundleID id = bundle->GetId ();
  
  std::lock_guard<std::mutex> lock (m_mutex);
  
  // Check if we've reached the maximum number of bundles
  if (m_bundles.size() >= m_maxBundles)
    {
      // Could implement bundle eviction policy here
      // For now, just reject the new bundle
      return false;
    }
  
  // Store the bundle
  m_bundles[id] = bundle;
  m_pushCount++;
  
  return true;
}

std::optional<Ptr<Bundle>> 
MemoryBundleStore::Get (const BundleID& id) const
{
  std::lock_guard<std::mutex> lock (m_mutex);
  
  auto it = m_bundles.find (id);
  if (it != m_bundles.end ())
    {
      const_cast<MemoryBundleStore*>(this)->m_getCount++;
      return it->second;
    }
  
  return std::nullopt;
}

bool 
MemoryBundleStore::Has (const BundleID& id) const
{
  std::lock_guard<std::mutex> lock (m_mutex);
  
  return m_bundles.find (id) != m_bundles.end ();
}

bool 
MemoryBundleStore::Remove (const BundleID& id)
{
  std::lock_guard<std::mutex> lock (m_mutex);
  
  auto it = m_bundles.find (id);
  if (it != m_bundles.end ())
    {
      m_bundles.erase (it);
      m_removeCount++;
      return true;
    }
  
  return false;
}

std::vector<Ptr<Bundle>> 
MemoryBundleStore::GetAll () const
{
  std::lock_guard<std::mutex> lock (m_mutex);
  
  std::vector<Ptr<Bundle>> result;
  result.reserve (m_bundles.size ());
  
  for (const auto& pair : m_bundles)
    {
      result.push_back (pair.second);
    }
  
  return result;
}

std::vector<Ptr<Bundle>> 
MemoryBundleStore::Query (std::function<bool(Ptr<Bundle>)> predicate) const
{
  std::lock_guard<std::mutex> lock (m_mutex);
  
  std::vector<Ptr<Bundle>> result;
  
  for (const auto& pair : m_bundles)
    {
      if (predicate (pair.second))
        {
          result.push_back (pair.second);
        }
    }
  
  return result;
}

size_t 
MemoryBundleStore::Count () const
{
  std::lock_guard<std::mutex> lock (m_mutex);
  
  return m_bundles.size ();
}

size_t 
MemoryBundleStore::Cleanup ()
{
  std::lock_guard<std::mutex> lock (m_mutex);
  
  size_t removedCount = 0;
  
  for (auto it = m_bundles.begin (); it != m_bundles.end ();)
    {
      if (IsExpired (it->second))
        {
          it = m_bundles.erase (it);
          m_removeCount++;
          removedCount++;
        }
      else
        {
          ++it;
        }
    }
  
  return removedCount;
}

std::string 
MemoryBundleStore::GetStats () const
{
  std::lock_guard<std::mutex> lock (m_mutex);
  
  std::stringstream ss;
  
  Time uptime = Simulator::Now () - m_creationTime;
  
  ss << "MemoryBundleStore(";
  ss << "count=" << m_bundles.size ();
  ss << ", pushed=" << m_pushCount;
  ss << ", retrieved=" << m_getCount;
  ss << ", removed=" << m_removeCount;
  ss << ", maxBundles=" << m_maxBundles;
  ss << ", uptime=" << uptime.GetSeconds () << "s";
  ss << ")";
  
  return ss.str ();
}

bool 
MemoryBundleStore::IsExpired (Ptr<Bundle> bundle) const
{
  Time creationTime = bundle->GetPrimaryBlock ().GetCreationTimestamp ().ToTime ();
  Time lifetime = bundle->GetPrimaryBlock ().GetLifetime ();
  Time expirationTime = creationTime + lifetime;
  
  return Simulator::Now () > expirationTime;
}

} // namespace dtn7

} // namespace ns3