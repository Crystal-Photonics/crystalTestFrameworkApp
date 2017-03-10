#ifndef EXPORT_H
#define EXPORT_H

#if 0
#  if defined EXPORT_LIBRARY
#    define EXPORT Q_DECL_EXPORT
#  elif EXPORT_APPLICATION
#    define EXPORT Q_DECL_IMPORT
#  else
#    error "#define EXPORT_LIBRARY or EXPORT_APPLICATION"
#  endif
#else
#  define EXPORT
#endif

#endif // EXPORT_H
