add_subdirectory(cxx)
add_subdirectory(grammar)

set(LLVM_LINK_COMPONENTS Support)

add_clang_library(clangPseudo
  Bracket.cpp
  DirectiveTree.cpp
  Forest.cpp
  GLR.cpp
  Lex.cpp
  Token.cpp

  LINK_LIBS
  clangBasic
  clangLex
  clangPseudoGrammar
  )
