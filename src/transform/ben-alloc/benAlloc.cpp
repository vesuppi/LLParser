//
// Created by tzhou on 8/10/17.
//

#include <fstream>
#include <set>
#include <algorithm>
#include <utilities/macros.h>
#include <asmParser/sysDict.h>
#include <passes/pass.h>
#include <di/diEssential.h>
#include <ir/irEssential.h>
#include <utilities/strings.h>
#include <peripheral/sysArgs.h>
#include <inst/instEssential.h>
#include <asmParser/irBuilder.h>


struct MFunc {
  string old_name;
  string new_name;
  bool add_id;

  MFunc(string oldname, string newname, bool addid):
    old_name(oldname), new_name(newname), add_id(addid) {}
};

class BenAllocPass: public Pass {
  /* language specific stuff */
  string _lang;
  std::vector<MFunc*> _alloc_set;
  std::vector<MFunc*> _free_set;
  int _apid;
  bool _use_indi;
  bool _print_stats;
  bool _print_callsites;
  int _new_allocs;
  int _new_frees;
  std::ofstream _site_printer;
  string _guide_file;
  std::map<int, int> _ap_map;
public:
  BenAllocPass() {
    set_is_module_pass();

    _lang = "all";
    init();
  }

  BenAllocPass(string lang) {
    set_is_module_pass();

    _lang = lang;
    init();
  }

  void init() {
    _use_indi = false;
    _apid = 1;
    _print_stats = true;
    _print_callsites = true;
    _new_allocs = 0;
    _new_frees = 0;
  }

  const std::vector<MFunc *> &alloc_set() const {
    return _alloc_set;
  }

  void set_alloc_set(const std::vector<MFunc *> &_alloc_set) {
    BenAllocPass::_alloc_set = _alloc_set;
  }

  const std::vector<MFunc *> &free_set() const {
    return _free_set;
  }

  void set_free_set(const std::vector<MFunc *> &_free_set) {
    BenAllocPass::_free_set = _free_set;
  }

  bool use_indi() const {
    return _use_indi;
  }

  void set_use_indi(bool _use_indi) {
    BenAllocPass::_use_indi = _use_indi;
  }

  void set_lang(string lang) {
    guarantee(lang == "c" || lang == "cpp" || lang == "fortran" || lang == "all", "");
    _lang = lang;
  }

  const string& lang() {
    return _lang;
  }

  void do_initialization() override {
    if (has_argument("lang")) {
      _lang = get_argument("lang");
    }
    if (has_argument("indi")) {
      _use_indi = true;
    }
    if (has_argument("guide")) {
      _guide_file = get_argument("guide");
    }

    init_guide();
    init_lang();
  }

  void init_guide() {
    if (_guide_file.empty()) {
      return;
    }

    std::ifstream ifs(_guide_file);
    string line;
    while (std::getline(ifs, line)) {
      int pos = line.find(' ');
      int n1 = std::stoi(line.substr(0, pos));
      int n2 = std::stoi(line.substr(pos+1));
      _ap_map[n1] = n2;
      zpd(n1)
      zpd(n2)
    }
  }

  void init_lang() {
    if (_lang == "c" || _lang == "cpp" || _lang == "all") {
      _alloc_set.push_back(new MFunc("malloc", "ben_malloc", true));
      _alloc_set.push_back(new MFunc("calloc", "ben_calloc", true));
      _alloc_set.push_back(new MFunc("realloc", "ben_realloc", true));
      _free_set.push_back(new MFunc("free", "ben_free", false));
    }

    if (_lang == "cpp" || _lang == "all") {
      _alloc_set.push_back(new MFunc("_Znam", "ben_malloc", true));
      _alloc_set.push_back(new MFunc("_Znwm", "ben_malloc", true));
      _free_set.push_back(new MFunc("_ZdaPv", "ben_free", false));
      _free_set.push_back(new MFunc("_ZdlPv", "ben_free", false));
    }

    if (_lang == "flang" || _lang == "all") {
      _alloc_set.push_back(new MFunc("f90_alloc", "f90_ben_alloc", true));
      _alloc_set.push_back(new MFunc("f90_alloc03", "f90_ben_alloc03", true));
      _alloc_set.push_back(new MFunc("f90_alloc03_chk", "f90_ben_alloc03_chk", true));
      _alloc_set.push_back(new MFunc("f90_alloc04", "f90_ben_alloc04", true));
      _alloc_set.push_back(new MFunc("f90_alloc04_chk", "f90_ben_alloc04_chk", true));

      _alloc_set.push_back(new MFunc("f90_kalloc", "f90_ben_kalloc", true));
      _alloc_set.push_back(new MFunc("f90_calloc", "f90_ben_calloc", true));
      _alloc_set.push_back(new MFunc("f90_calloc03", "f90_ben_calloc03", true));
      _alloc_set.push_back(new MFunc("f90_calloc04", "f90_ben_calloc04", true));
      _alloc_set.push_back(new MFunc("f90_kcalloc", "f90_ben_kcalloc", true));
      _alloc_set.push_back(new MFunc("f90_ptr_alloc", "f90_ben_ptr_alloc", true));
      _alloc_set.push_back(new MFunc("f90_ptr_alloc03", "f90_ben_ptr_alloc03", true));
      _alloc_set.push_back(new MFunc("f90_ptr_alloc04", "f90_ben_ptr_alloc04", true));
      _alloc_set.push_back(new MFunc("f90_ptr_src_alloc03", "f90_ben_ptr_src_alloc03", true));
      _alloc_set.push_back(new MFunc("f90_ptr_src_alloc04", "f90_ben_ptr_src_alloc04", true));
      _alloc_set.push_back(new MFunc("f90_ptr_src_calloc03", "f90_ben_ptr_src_calloc03", true));
      _alloc_set.push_back(new MFunc("f90_ptr_src_calloc04", "f90_ben_ptr_src_calloc04", true));
      _alloc_set.push_back(new MFunc("f90_ptr_kalloc", "f90_ben_ptr_kalloc", true));
      _alloc_set.push_back(new MFunc("f90_ptr_calloc", "f90_ben_ptr_calloc", true));
      _alloc_set.push_back(new MFunc("f90_ptr_calloc03", "f90_ben_ptr_calloc03", true));
      _alloc_set.push_back(new MFunc("f90_ptr_calloc04", "f90_ben_ptr_calloc04", true));
      _alloc_set.push_back(new MFunc("f90_ptr_kcalloc", "f90_ben_ptr_kcalloc", true));
      _alloc_set.push_back(new MFunc("f90_auto_allocv", "f90_ben_auto_allocv", true));
      _alloc_set.push_back(new MFunc("f90_auto_alloc", "f90_ben_auto_alloc", true));
      _alloc_set.push_back(new MFunc("f90_auto_alloc04", "f90_ben_auto_alloc04", true));
      _alloc_set.push_back(new MFunc("f90_auto_calloc", "f90_ben_auto_calloc", true));
      _alloc_set.push_back(new MFunc("f90_auto_calloc04", "f90_ben_auto_calloc04", true));

      _free_set.push_back(new MFunc("f90_dealloc", "f90_ben_dealloc", false));
      _free_set.push_back(new MFunc("f90_dealloc03", "f90_ben_dealloc03", false));
      _free_set.push_back(new MFunc("f90_dealloc_mbr", "f90_ben_dealloc_mbr", false));
      _free_set.push_back(new MFunc("f90_dealloc_mbr03", "f90_ben_dealloc_mbr03", false));
      _free_set.push_back(new MFunc("f90_deallocx", "f90_ben_deallocx", false));
      _free_set.push_back(new MFunc("f90_auto_dealloc", "f90_ben_auto_dealloc", false));
      // todo
    }
  }

  void insert_lsd(Module* module) {
    for (auto t: _alloc_set) {
      if (Strings::startswith(t->old_name, "f90_")) {
        insert_declaration(module, t->old_name, t->new_name, false);
      }
      else {
        insert_declaration(module, t->old_name, t->new_name, t->add_id);
      }

      if (_use_indi) {
        string indi_name = t->new_name;
        Strings::ireplace(indi_name, "ben_", "indi_");
        insert_declaration(module, t->old_name, indi_name, false);
      }
    }

    for (auto t: _free_set) {
      insert_declaration(module, t->old_name, t->new_name, t->add_id);
      if (_use_indi) {
        string indi_name = t->new_name;
        Strings::ireplace(indi_name, "ben_", "indi_");
        insert_declaration(module, t->old_name, indi_name, false);
      }
    }
  }

  void replace_alloc(Module* module) {
    for (auto t: _alloc_set) {
      if (Function* f = module->get_function(t->old_name)) {
        for (auto I: f->caller_list()) {
          /* flang always uses indirect call, so update the bitcast here
           * including both callee, and the type
           */
          bool flang_alloc = false;
          if (Strings::startswith(t->old_name, "f90_")) {
            flang_alloc = true;
          }
          if (flang_alloc) {
            if (I->is_indirect_call()) {
              BitCastInst* bci = dynamic_cast<BitCastInst*>(I->chain_inst());
              if (!bci) {
                printf("Chain instruction not found for: %s\n", I->raw_c_str());
              }
              guarantee(bci, "");
              bci->update_raw_field("value", "@" + t->new_name);
              string ty2 = bci->get_raw_field("ty2");
              insert_i32_to_type(ty2);
              bci->update_raw_field("ty2", ty2);
            }
            else {
              I->replace_callee(t->new_name);
              /* hacky code */
              auto pos = I->raw_text().find(" to ");
              guarantee(pos != string::npos, "");
              auto pos1 = I->raw_text().find("(i8*", pos);
              guarantee(pos1 != string::npos, "");
              I->raw_text().insert(pos1+1, "i32, ");
            }
          }
          else {
            I->replace_callee(t->new_name);
          }

          int id = _apid++;
          if (_ap_map.find(id) != _ap_map.end()) {
            //zpl("map %d to %d", id, _ap_map[id]);
            id = _ap_map[id];
          }
          string new_args = "i32 " + std::to_string(id) + ", " + I->get_raw_field("args");
          I->replace_args(new_args);

          if (flang_alloc) {
            string fnty = I->get_raw_field("fnty");
            insert_i32_to_type(fnty);
            I->update_raw_field("fnty", fnty);
            //I->dump();
          }

          /* print call site */
          if (_print_callsites) {
            DILocation* loc = I->debug_loc();
            _site_printer << id << " 1 " << t->new_name << std::endl;
            _site_printer << "(" << I->function()->name() << "+" << I->get_position_in_function() << ")"
                          << " " << loc->filename() << ":" << loc->line() << "\n" << std::endl;
          }
        }
      }
    }
  }

  void insert_i32_to_type(string& ty) {
    int replace_pos = ty.find('(');
    ty.replace(replace_pos, 1, "(i32, ");
  }

  void replace_free(Module* module) {
    string suffixes[4] = {" ", ",", ")", "("};
    for (auto F: module->function_list()) {
      for (auto B: F->basic_block_list()) {
        for (auto I: B->instruction_list()) {
          for (auto& suf: suffixes) {
            for (auto& t: _free_set) {
              string old = "@"+t->old_name+suf;
              if (I->raw_text().find(old) != string::npos) {
                Strings::ireplace(I->raw_text(), old, "@"+t->new_name+suf);
                _new_frees++;
              }
            }
          }
        }
      }
    }
  }

  void replace_indi(Module* module) {
    string suffixes[3] = {" ", ",", ")"};
    for (auto F: module->function_list()) {
      for (auto B: F->basic_block_list()) {
        for (auto I: B->instruction_list()) {
          for (auto& suf: suffixes) {
            //string targets[4] = {"malloc", "calloc", "realloc", "free"};
            for (auto& t: _alloc_set) {
              string old = "@"+t->old_name+suf;
              if (I->raw_text().find(old) != string::npos) {
                Strings::ireplace(I->raw_text(), old, "@indi_"+t->old_name+suf);
              }
            }
//                        for (auto& t: _free_set) {
//                            string old = "@"+t->old_name+suf;
//                            if (I->raw_text().find(old) != string::npos) {
//                                Strings::ireplace(I->raw_text(), old, "@indi_"+t->old_name+suf);
//                            }
//                        }
          }
        }
      }
    }
  }

  bool run_on_module(Module* module) override {
    if (_print_callsites) {
      _site_printer.open(module->name() + ".callsites.txt");
    }


    insert_lsd(module);
    replace_alloc(module);
    replace_free(module);

    string out = SysArgs::get_option("output");
    if (out.empty()) {
      out = SysDict::filename();
      if (Strings::contains(out, ".ll")) {
        Strings::ireplace(out, ".ll", ".ben.ll");
      }
      else {
        out += ".ben.ll";
      }
    }

    zpl("BenAlloc output to %s", out.c_str())
    module->print_to_file(out);

    if (_print_callsites) {
      _site_printer.close();
    }

    return true;
  }
//
//    void replace_free() {
//        for (auto t: _free_set) {
//            if (Function* free_fp = SysDict::module()->get_function(t->old_name)) {
//                for (auto ci: free_fp->caller_list()) {
//                    ci->replace_callee(t->new_name);
//                }
//            }
//        }
//
//        //todo: non-dirty way
//        if (!_use_indi) {
//            return;
//        }
//
//        string suffixes[3] = {" ", ",", ")"};
//        for (auto F: SysDict::module()->function_list()) {
//            for (auto B: F->basic_block_list()) {
//                for (auto I: B->callinst_list()) {
//                    for (auto& suf: suffixes) {
//                        //string targets[4] = {"malloc", "calloc", "realloc", "free"};
//                        for (auto& t: _alloc_set) {
//                            string old = "@"+t->old_name+suf;
//                            if (I->raw_text().find(old) != string::npos) {
//                                Strings::ireplace(I->raw_text(), old, "@indi_"+t->old_name+suf);
//                            }
//                        }
//                        for (auto& t: _free_set) {
//                            string old = "@"+t->old_name+suf;
//                            if (I->raw_text().find(old) != string::npos) {
//                                Strings::ireplace(I->raw_text(), old, "@indi_"+t->old_name+suf);
//                            }
//                        }
//                    }
//                }
//            }
//        }
//    }

  void insert_declaration(Module* m, string oldname, string newname, bool add_id=true) {
    Function* func = m->get_function(oldname);

    if (func == NULL) {
      return;
    }

    /* _Znwm is defined in omnetpp */
    //guarantee(func->is_external(), "malloc family should be external: %s", func->raw_c_str());

    if (m->get_function(newname)) {
      return; // return if already inserted
    }

    /* manipulate the text */
    string text = func->raw_text();
    string old_call = oldname + '(';
    string new_call = newname + "(i32, ";
    //if (!add_id || Strings::startswith(oldname, "f90_")) {  // a bit funky
    if (!add_id) {
      new_call = newname + '(';
    }

    Strings::ireplace(text, old_call, new_call);
    Function* newfunc = IRBuilder::create_function_declaration(text);
    m->insert_function_after(func, newfunc);
    //printf("<lsda>: insert %s to module %s\n", newfunc->name_as_c_str(), m->name_as_c_str());
  }

};


REGISTER_PASS(BenAllocPass);
