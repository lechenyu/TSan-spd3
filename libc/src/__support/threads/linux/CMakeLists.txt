add_header_library(
  futex_word_type
  HDRS
    futex_word.h
)

if(NOT TARGET libc.src.__support.OSUtil.osutil)
  return()
endif()

add_header_library(
  mutex
  HDRS
    mutex.h
  DEPENDS
    .futex_word_type
    libc.include.sys_syscall
    libc.src.__support.CPP.atomic
    libc.src.__support.OSUtil.osutil
    libc.src.__support.threads.mutex_common
)

add_header_library(
  thread
  HDRS
    thread.h
  DEPENDS
    .futex_word_type
    libc.include.sys_syscall
    libc.src.__support.CPP.atomic
    libc.src.__support.CPP.error
    libc.src.__support.threads.thread_attrib
)
