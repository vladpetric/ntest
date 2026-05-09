class CBook;
class CCache;
class CEvaluator;
class CMPCStats;

extern CBook* book;

// search parameters
extern CCache* cache;
extern CEvaluator* evaluator;
extern CMPCStats* mpcs;

extern int hBookRead;

#ifdef _OPENMP
#pragma omp threadprivate(book, cache, evaluator, mpcs, hBookRead)
#endif
