#include "ToyList.h"
#include "IToyUpdatable.h"
#include "../Cabinet.h"
#include "../../Log.h"
#include "../../general/StringExtensions.h"

#include <algorithm>
#include <stdexcept>

namespace DOF
{

ToyList::ToyList() { }

ToyList::~ToyList()
{
   for (IToy* toy : *this)
      delete toy;
   clear();
}

void ToyList::Init(Cabinet* cabinet)
{
   Log::Write(StringExtensions::Build("Initializing {0} toys", std::to_string(size())));

   for (IToy* toy : *this)
   {
      if (toy != nullptr)
      {
         try
         {
            toy->Init(cabinet);
         }
         catch (const std::exception& e)
         {
            Log::Exception(StringExtensions::Build("Error initializing toy {0}: {1}", toy->GetName(), e.what()));
         }
      }
   }
}

void ToyList::Reset()
{
   Log::Write(StringExtensions::Build("Resetting {0} toys", std::to_string(size())));

   for (IToy* toy : *this)
   {
      if (toy != nullptr)
      {
         try
         {
            toy->Reset();
         }
         catch (const std::exception& e)
         {
            Log::Exception(StringExtensions::Build("Error resetting toy {0}: {1}", toy->GetName(), e.what()));
         }
      }
   }
}

void ToyList::Finish()
{
   Log::Write(StringExtensions::Build("Finishing {0} toys", std::to_string(size())));

   std::string failures;
   bool failed = false;
   for (IToy* toy : *this)
   {
      if (toy != nullptr)
      {
         std::string error;
         try
         {
            toy->Finish();
            continue;
         }
         catch (const std::exception& e)
         {
            error = e.what();
         }
         catch (...)
         {
            error = "unknown exception";
         }
         failed = true;
         Log::Exception(StringExtensions::Build("Error finishing toy {0}: {1}", toy->GetName(), error));
         failures += (failures.empty() ? "" : "; ") + toy->GetName() + ": " + error;
      }
   }
   if (failed)
      throw std::runtime_error("Toys failed to finish: " + failures);
}

void ToyList::UpdateToys()
{
   for (IToy* toy : *this)
   {
      if (toy != nullptr)
      {
         IToyUpdatable* updatableToy = dynamic_cast<IToyUpdatable*>(toy);
         if (updatableToy != nullptr)
         {
            try
            {
               updatableToy->UpdateToy();
            }
            catch (const std::exception& e)
            {
               Log::Exception(StringExtensions::Build("Error updating toy {0}: {1}", toy->GetName(), e.what()));
            }
         }
      }
   }
}

void ToyList::UpdateOutputs() { UpdateToys(); }

IToy* ToyList::FindByName(const std::string& name) const
{
   for (IToy* toy : *this)
   {
      if (toy != nullptr && toy->GetName() == name)
         return toy;
   }
   return nullptr;
}

void ToyList::AddToy(IToy* toy)
{
   if (toy == nullptr)
      return;

   if (FindByName(toy->GetName()) != nullptr)
   {
      Log::Warning(StringExtensions::Build("Toy with name {0} already exists", toy->GetName()));
      return;
   }

   push_back(toy);
   Log::Debug(StringExtensions::Build("Added toy: {0}", toy->GetName()));
}

bool ToyList::RemoveToy(IToy* toy, bool deleteToy)
{
   if (toy == nullptr)
      return false;

   auto it = std::find(begin(), end(), toy);
   if (it != end())
   {
      Log::Write(StringExtensions::Build("Removed toy: {0}", toy->GetName()));
      erase(it);

      if (deleteToy)
         delete toy;
      return true;
   }

   return false;
}

bool ToyList::RemoveToy(const std::string& name, bool deleteToy)
{
   IToy* toy = FindByName(name);
   if (toy != nullptr)
      return RemoveToy(toy, deleteToy);

   return false;
}

bool ToyList::Contains(const std::string& name) const { return FindByName(name) != nullptr; }

void ToyList::WriteXml() { Log::Write("ToyList::WriteXml not yet implemented"); }

void ToyList::ReadXml() { Log::Write("ToyList::ReadXml not yet implemented"); }

}
