//
// Created by kumagi on 18/06/06.
//

#ifndef PROJECT_DEBUG_HPP
#define PROJECT_DEBUG_HPP

#ifdef LLVM_IS_DEBUG_BUILD
 # define LLVM_DUMP(obj) (obj)->dump()
#else
 # define LLVM_DUMP(obj)
#endif

#endif //PROJECT_DEBUG_HPP
