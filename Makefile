CC =	g++

COMMONS_INC     = .

CCOPT =		
INCLS =		-I/usr/include/mysql -I$(COMMONS_INC) -g
CFLAGS =	$(CCOPT) $(INCLS) 
LIBS =		-lmysqlclient -lnsl -lm -lz -lc -L/usr/lib64 -g -lpthread -Wwrite-strings

DEST_DIR	= .

COMMON_SRC	= $(COMMONS_INC)/Utils.c $(COMMONS_INC)/TextUtils.c $(COMMONS_INC)/DateUtils.c $(COMMONS_INC)/Tokenizer.c $(COMMONS_INC)/md5.c $(COMMONS_INC)/DBUtils.c
PELLET_OBJ	= Reaziener.o KRSSLoader.o ExpressionNode.o KnowledgeBase.o TBox.o ABox.o ReazienerUtils.o Expressivity.o Node.o Params.o Tu.o Tg.o DependencySet.o TBoxBase.o TermDefinition.o SizeEstimate.o CompletionQueue.o Individual.o Role.o RBox.o Clash.o DependencyIndex.o Dependency.o DependencyEntry.o Literal.o Edge.o EdgeList.o QueueElement.o Taxonomy.o CachedNode.o ConceptCache.o TransitionGraph.o RoleTaxonomyBuilder.o State.o Transition.o CompletionStrategy.o SROIQStrategy.o SROIQIncStrategy.o Branch.o SHOIQStrategy.o SHOINStrategy.o EmptySHNStrategy.o TaxonomyBuilder.o TaxonomyNode.o NodeMerge.o Blocking.o Datatype.o ZList.o TaxonomyPrinter.o

reaziener : $(COMMON_SRC) $(PELLET_OBJ)
	$(CC) $(COMMON_SRC) $(PELLET_OBJ) $(LIBS) $(CFLAGS) -o $(DEST_DIR)/$@
