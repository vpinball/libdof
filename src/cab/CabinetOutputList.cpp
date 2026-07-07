#include "CabinetOutputList.h"
#include "Cabinet.h"
#include "out/OutputControllerList.h"
#include "out/IOutputController.h"
#include "out/OutputList.h"
#include "../general/StringExtensions.h"
#include <stdexcept>
#include <algorithm>

namespace DOF
{

CabinetOutputList::CabinetOutputList(Cabinet* cabinet)
   : m_cabinet(cabinet)
{
}

CabinetOutputList::~CabinetOutputList() { }

OutputControllerList* CabinetOutputList::GetOutputControllers() const { return m_cabinet ? m_cabinet->GetOutputControllers() : nullptr; }

IOutput* CabinetOutputList::GetByName(const std::string& name) { return const_cast<IOutput*>(static_cast<const CabinetOutputList*>(this)->GetByName(name)); }

const IOutput* CabinetOutputList::GetByName(const std::string& name) const
{
   std::string outputName = "";

   std::vector<std::string> nameParts = StringExtensions::Split(name, { '\\', '/' });
   if (nameParts.size() == 2)
   {
      OutputControllerList* controllers = GetOutputControllers();
      if (controllers && controllers->Contains(nameParts[0]))
      {
         outputName = nameParts[1];
      }
   }
   else
   {
      outputName = name;
   }

   std::vector<std::string> dotParts = StringExtensions::Split(outputName, { '.' });
   if (dotParts.size() == 2)
   {
      OutputControllerList* controllers = GetOutputControllers();
      int nr = 0;
      if (controllers && controllers->Contains(dotParts[0]) && StringExtensions::TryParseInt(dotParts[1], nr))
      {
         for (IOutputController* controller : *controllers)
         {
            if (controller && controller->GetName() == dotParts[0])
            {
               const OutputList* outputs = controller->GetOutputs();
               if (outputs)
               {
                  const IOutput* o = outputs->FindByNumber(nr);
                  if (o != nullptr)
                     return o;
               }
               break;
            }
         }
      }
   }

   OutputControllerList* controllers = GetOutputControllers();
   if (controllers)
   {
      for (IOutputController* controller : *controllers)
      {
         if (controller)
         {
            const OutputList* outputs = controller->GetOutputs();
            if (outputs && outputs->Contains(outputName))
            {
               return outputs->GetByName(outputName);
            }
         }
      }
   }

   return nullptr;
}

bool CabinetOutputList::Contains(const std::string& name) const { return GetByName(name) != nullptr; }

bool CabinetOutputList::Contains(IOutput* output) const
{
   OutputControllerList* controllers = GetOutputControllers();
   if (controllers)
   {
      for (IOutputController* controller : *controllers)
      {
         if (controller)
         {
            const OutputList* outputs = controller->GetOutputs();
            if (outputs && outputs->Contains(output))
            {
               return true;
            }
         }
      }
   }
   return false;
}

IOutput* CabinetOutputList::operator[](const std::string& name)
{
   IOutput* output = GetByName(name);
   if (!output)
   {
      throw std::out_of_range("Output with name '" + name + "' not found");
   }
   return output;
}

const IOutput* CabinetOutputList::operator[](const std::string& name) const
{
   const IOutput* output = GetByName(name);
   if (!output)
   {
      throw std::out_of_range("Output with name '" + name + "' not found");
   }
   return output;
}

IOutput* CabinetOutputList::operator[](int index) { return const_cast<IOutput*>(static_cast<const CabinetOutputList*>(this)->operator[](index)); }

const IOutput* CabinetOutputList::operator[](int index) const
{
   int count = GetCount();
   if (index >= count)
   {
      throw std::out_of_range("Enumeration index of CabinetOutputList out of range");
   }

   OutputControllerList* controllers = GetOutputControllers();
   if (controllers)
   {
      int currentIndex = 0;
      for (IOutputController* controller : *controllers)
      {
         if (controller)
         {
            const OutputList* outputs = controller->GetOutputs();
            if (outputs)
            {
               if (index < currentIndex + static_cast<int>(outputs->size()))
               {
                  return (*outputs)[index - currentIndex];
               }
               currentIndex += static_cast<int>(outputs->size());
            }
         }
      }
   }

   throw std::out_of_range("Enumeration index of CabinetOutputList out of range");
}

int CabinetOutputList::GetCount() const
{
   int count = 0;
   OutputControllerList* controllers = GetOutputControllers();
   if (controllers)
   {
      for (IOutputController* controller : *controllers)
      {
         if (controller)
         {
            const OutputList* outputs = controller->GetOutputs();
            if (outputs)
            {
               count += static_cast<int>(outputs->size());
            }
         }
      }
   }
   return count;
}

void CabinetOutputList::ConnectOutputsToControllers() { }

CabinetOutputList::Iterator::Iterator(CabinetOutputList* list, int index)
   : m_list(list)
   , m_index(index)
{
}

IOutput* CabinetOutputList::Iterator::operator*() { return (*m_list)[m_index]; }

CabinetOutputList::Iterator& CabinetOutputList::Iterator::operator++()
{
   ++m_index;
   return *this;
}

bool CabinetOutputList::Iterator::operator!=(const Iterator& other) const { return m_list != other.m_list || m_index != other.m_index; }

bool CabinetOutputList::Iterator::operator==(const Iterator& other) const { return m_list == other.m_list && m_index == other.m_index; }

CabinetOutputList::ConstIterator::ConstIterator(const CabinetOutputList* list, int index)
   : m_list(list)
   , m_index(index)
{
}

const IOutput* CabinetOutputList::ConstIterator::operator*() const { return (*m_list)[m_index]; }

CabinetOutputList::ConstIterator& CabinetOutputList::ConstIterator::operator++()
{
   ++m_index;
   return *this;
}

bool CabinetOutputList::ConstIterator::operator!=(const ConstIterator& other) const { return m_list != other.m_list || m_index != other.m_index; }

bool CabinetOutputList::ConstIterator::operator==(const ConstIterator& other) const { return m_list == other.m_list && m_index == other.m_index; }

CabinetOutputList::Iterator CabinetOutputList::begin() { return Iterator(this, 0); }

CabinetOutputList::Iterator CabinetOutputList::end() { return Iterator(this, GetCount()); }

CabinetOutputList::ConstIterator CabinetOutputList::begin() const { return ConstIterator(this, 0); }

CabinetOutputList::ConstIterator CabinetOutputList::end() const { return ConstIterator(this, GetCount()); }

CabinetOutputList::ConstIterator CabinetOutputList::cbegin() const { return ConstIterator(this, 0); }

CabinetOutputList::ConstIterator CabinetOutputList::cend() const { return ConstIterator(this, GetCount()); }

}
