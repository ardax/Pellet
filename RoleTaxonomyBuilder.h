#ifndef _ROLE_TAXONOMY_BUILDER_
#define _ROLE_TAXONOMY_BUILDER_

class RBox;
class Taxonomy;

class RoleTaxonomyBuilder
{
 public:
  RoleTaxonomyBuilder(RBox* pRBox);

  Taxonomy* classify();

  RBox* m_pRBox;
};

#endif
