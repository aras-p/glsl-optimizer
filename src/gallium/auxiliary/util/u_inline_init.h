#ifndef U_INLINE_INIT_H_
#define U_INLINE_INIT_H_

#define UTIL_INLINE_INIT(m) \
   extern boolean m##_inited; \
   extern void m##_do_init(void); \
   static INLINE void m##_init(void) \
   { \
      if(!m##_inited) { \
         m##_do_init(); \
         m##_inited = TRUE; \
      } \
   }

#endif /* U_INLINE_INIT_H_ */
