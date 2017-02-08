#include "SHOIQStrategy.h"
#include "Blocking.h"

SHOIQStrategy::SHOIQStrategy(ABox* pABox) : SROIQStrategy(pABox, (new OptimizedDoubleBlocking()))
{

}
