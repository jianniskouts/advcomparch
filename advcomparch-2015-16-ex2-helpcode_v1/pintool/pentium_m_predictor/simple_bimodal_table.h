#ifndef BIMODAL_TABLE_H
#define BIMODAL_TABLE_H

#include "../branch_predictor.h"
#include "saturating_predictor.h"

class SimpleBimodalTable : BranchPredictor
{

public:

   SimpleBimodalTable(UINT32 entries)
      : m_num_entries(entries)
      , m_table(entries, 0)
   {
      reset();
      m_mask = 0;
      for (unsigned int i = 0 ; i < ilog2(m_num_entries) ; i++)
      {
         m_mask |= (1L<<i);
      }
   }

   virtual bool predict(ADDRINT ip, ADDRINT target)
   {
      UINT32 index = ip & m_mask;
      return (m_table[index].predict());
   }

   virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target)
   {
      UINT32 index = ip & m_mask;
      if (actual)
      {
         ++m_table[index];
      }
      else
      {
         --m_table[index];
      }
   }

   virtual string getName() { return "SimpleBimodal"; }

   void reset()
   {
      for (unsigned int i = 0 ; i < m_num_entries ; i++) {
         m_table[i].reset();
      }
   }

private:

   template<typename Addr>
   Addr ilog2(Addr n)
   {
      Addr i;
      for(i=0;n>0;n>>=1,i++) {}
      return i-1;
   }

private:

   UINT32 m_num_entries;
   ADDRINT m_mask;
   std::vector<SaturatingPredictor<2> > m_table;

};

#endif /* BIMODAL_TABLE_H */
