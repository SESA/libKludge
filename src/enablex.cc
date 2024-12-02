/* head - copy first part of files. */

/* See Makefile for compilation details. */

/*
   Copyright (C) 1999-2009 Free Software Foundation, Inc.

   This file is part of GNU Bash.
   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"

#include "bashtypes.h"
#include "posixstat.h"
#include "filecntl.h"

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include "bashansi.h"

#include <stdio.h>
#include <errno.h>
#include <vector>
#include "chartypes.h"

#include <cassert>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <elf.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <unistd.h>


#include "builtins.h"
#include "shell.h"
#include "bashgetopt.h"
#include "common.h"

#include "dyn_loader.h"
#include "enablex.h"

#include <unistd.h> 
#include <setjmp.h>
#include <ostream>

#if !defined (errno)
extern int errno;
#endif

void write_sloader_dummy_to_secure_tls_space();

char exename[] = "enablex";

std::vector<std::shared_ptr<DynLoader>> dynloaders;
std::vector<std::string> names;

Elf64_Addr global_next_base_addr_ = 0x140'0000;

BUILTINS_LIST * bl_head = NULL;

Elf64_Half GetEType(const std::filesystem::path& filepath) {
    int fd = open(filepath.c_str(), O_RDONLY);
    CHECK(fd >= 0);

    size_t size = lseek(fd, 0, SEEK_END);
    CHECK_GT(size, 8UL + 16UL);

    size_t mapped_size = (size + 0xfff) & ~0xfff;

    char* p = (char*)mmap(NULL, mapped_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fd, 0);
    LOG(FATAL) << LOG_BITS(mapped_size) << LOG_BITS(size) << LOG_KEY(filepath);
    CHECK(p != MAP_FAILED);

    Elf64_Ehdr* ehdr = reinterpret_cast<Elf64_Ehdr*>(p);
    return ehdr->e_type;
}

LIST_INFO convert_word_list_to_argv(WORD_LIST *list) {
    // Count the number of elements in the list
    int count = 0;
    WORD_LIST *current = list;
    while (current) {
        count++;
        current = current->next;
    }

    // Allocate memory for argv (count + 1 for NULL termination)
    char **argv = new char*[count + 1];
    if (!argv) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    // Populate argv with the word strings
    current = list;
    for (int i = 0; i < count; i++) {
        if (current->word && current->word->word) {
            argv[i] = current->word->word;
        } else {
            argv[i] = NULL; // Handle missing or NULL word pointers gracefully
        }
        current = current->next;
    }

    // Null-terminate the argv array
    argv[count] = NULL;

    LIST_INFO info;
    info.argc = count;
    info.argv = argv;

    return info;
}

ENABLEX_BUILTIN* load_link_binary(const char* const path) {
    std::string argv0 = path;
    std::filesystem::path fullpath;

    ENABLEX_BUILTIN* retVal = new ENABLEX_BUILTIN();

    if (argv0[0] == '.' || argv0.find("/") != std::string::npos) {
        fullpath = std::filesystem::path(argv0);
    } else {
        std::vector<std::string> path_dirs = SplitWith(std::string(getenv("PATH")), ":");
        for (const auto& dir : path_dirs) {
            std::filesystem::path p = std::filesystem::path(dir) / argv0;
            if (std::filesystem::exists(p) && std::filesystem::is_regular_file(p)) {
                fullpath = p;
                break;
            }
        }
    }

    LOG(INFO) << LOG_KEY(fullpath);
    CHECK(std::filesystem::exists(fullpath));

    retVal->name = fullpath.filename();
    names.push_back(retVal->name);

    


    Elf64_Half etype = GetEType(fullpath);
    if (etype == ET_DYN || etype == ET_EXEC) {
        write_sloader_dummy_to_secure_tls_space();

        auto _dynloader = std::make_shared<DynLoader>(fullpath, global_next_base_addr_);
        dynloaders.push_back(_dynloader);

        retVal->DynLoaderP = &dynloaders[dynloaders.size() - 1];


        _dynloader->LoadDependingLibs(fullpath);
        _dynloader->Relocate();

        if (_dynloader->next_base_addr_ > global_next_base_addr_)
          global_next_base_addr_ = _dynloader->next_base_addr_;

    } else {
        LOG(FATAL) << "Unsupported etype = " << LOG_KEY(etype);
        std::abort();
    }

    return retVal;
}

void execute_main(ENABLEX_BUILTIN* eb, int argc, char* const argv[], char** envp) {

    std::shared_ptr<DynLoader> *dynloader = static_cast<std::shared_ptr<DynLoader>*>(eb->DynLoaderP);

    for (int i = 0; i < argc; i++) {
      printf("argv[%d] = %s\n", i, argv[i]);
    }    
    
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++) {
        args.emplace_back(argv[i]);
    }

    std::vector<std::string> envs;
    for (char** env = envp; *env != 0; env++) {
        envs.emplace_back(*env);
    }

    if (setjmp(jumpBuffer) == 0) {
      (*dynloader)->Execute(args, envs);
    }
    std::cout << "came back through setjmp" << std::endl;

}



int enablex_builtin (WORD_LIST *list) {
  int nline, opt, rval;

  char *t;
  if (strcmp(list->word->word, "-V") == 0) {
    logstream = std::cout;
    list = list->next;
  }

  LIST_INFO info = convert_word_list_to_argv(list);
  
  // Print argv
  for (int i = 0; info.argv[i] != NULL; i++) {
      printf("argv[%d] = %s\n", i, info.argv[i]);
  }


  if (strcmp(list->word->word, "load") == 0) {
    ENABLEX_BUILTIN* b = load_link_binary(info.argv[1]);

    BUILTINS_LIST * newHead = new BUILTINS_LIST();
    newHead->_this = b;
    newHead->next = bl_head;

    bl_head = newHead;

    printf("New binary has been loaded: %s, %p\n", bl_head->_this->name.c_str(), bl_head->_this->name.c_str());
    exename[0] = 'U';

  }
  else if (strcmp(list->word->word, "run") == 0) {
    
    std::cout << "Exename: " << exename << std::endl;

    ENABLEX_BUILTIN * my_bi = NULL;
    for (BUILTINS_LIST * my_head = bl_head; my_head != NULL; my_head = my_head->next) {
      
      std::cout << "Checking " << my_head->_this->name.c_str() << std::endl;

      if (strcmp(my_head->_this->name.c_str(), info.argv[1]) == 0) {
        my_bi = my_head->_this;
        break;
      }
    }

    if (my_bi != NULL) {
      execute_main(my_bi, info.argc, info.argv, shell_environment);     
    }
    else {
      printf("No binary with name %s has been loaded\n", info.argv[1]);
    }

  }
  else {
    printf("Unknown command: %s\n", list->word->word);
  }

  std::cout << "ProcessId: " << getpid() << std::endl;
  printf("Exename %p\n", exename );

  // run_main(info.argc, info.argv, shell_environment); 



   
  return (rval);
}

int
enablex_builtin_load (char *name)
{
  return (1);
}



