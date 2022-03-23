#include "StdAfx.h"

#include "SubPhaseRetriever.h"

#include <CPS_ImportPVCareCoordCom.h>
#include <CPS_ImportCpsDataCommon.h>

/////////////////////////////////////////////////////////////////////////////
/// \fn		void CSubPhaseRetriever::GetIncludedSubPhases(IPhase& phase, std::list<IPhase*>& subPhases)
/// \brief		Retrieves all included sub-phases within the given phase. This does not handle treatment period sub-phases.
///
/// \return		void
///
/// \param[in]	IPhase& phase - The phase from which to retrieve all of the included sub-phases.
/// \param[out]	std::list<IPhase*>& subPhases - The phase's included sub-phases.
/// \owner		DB024248
/////////////////////////////////////////////////////////////////////////////
void CSubPhaseRetriever::GetIncludedSubPhases(IPhase& phase, std::list<IPhase*>& subPhases)
{
	IProVideObjectCollectionPtr pCompCollection = phase.GetComponentCollection();

	if (pCompCollection == NULL)
	{
		return;
	}

	long compTotal(0);
	pCompCollection->get_Count(&compTotal);

	for (long compCnt(compTotal); compCnt > 0; --compCnt)
	{
		_variant_t vComponent = pCompCollection->Value(CComVariant(compCnt));
		IComponentPtr pIComponent = vComponent.pdispVal;

		if (NULL != pIComponent)
		{
			if (TRUE == pIComponent->GetIncluded())
			{
				const CString sComponentTypeMeaning = (LPCTSTR)pIComponent->GetComponentTypeMean();

				if (sComponentTypeMeaning == "SUBPHASE")
				{
					IPhasePtr pSubPhase = pIComponent->GetSubphaseDispatch();

					if (pSubPhase != NULL)
					{
						subPhases.push_back(pSubPhase);
					}
				}
			}
		}

		VariantClear(&vComponent);
	}
}