#pragma once

#include "CoreMinimal.h"
#include "DMValuable.h"

/** Antique sets for generated maps. Stats mirror Docs/Tripo_Asset_List.md (S/M/L) and Tools/ue/build_level.py. */
namespace DMValuableCatalog
{
	/** The 15 antiques of the chapter 1 mansion; 20 kg+ items need two holders. */
	DREAMMOVERS_API TArray<FDMValuableSpec> MakeChapterOne();
}
