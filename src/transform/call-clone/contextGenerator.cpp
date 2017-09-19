//
// Created by tzhou on 9/18/17.
//

#include <asmParser/sysDict.h>
#include <di/diEssential.h>
#include <set>
#include "contextGenerator.h"


void ContextGenerator::generate(Module* module, string alloc, int nlevel) {
    Function* malloc = module->get_function(alloc);
    if (!malloc) {
        return;
    }

    std::ofstream ofs;
    ofs.open(SysDict::filepath() + "contexts.txt");

    for (auto ci: malloc->caller_list()) {
        XPath* path = new XPath;
        path->hotness = 0;
        path->path.push_back(ci);
        _paths.push_back(path);
    }

    while (--nlevel) {
        std::vector<XPath*> new_paths;
        for (auto xpath: _paths) {
            auto& context = xpath->path;
            auto tos = context[context.size()-1];
            for (auto ci: tos->function()->caller_list()) {
                XPath* nxp = new XPath();
                nxp->hotness = xpath->hotness;
                nxp->path = context;
                nxp->path.push_back(ci);
                new_paths.push_back(nxp);
            }
        }
        _paths = new_paths;
    }

    int id = 0;
    for (auto xpath: _paths) {
        ofs << id++ << " 0x0 " << alloc << std::endl;
        for (auto ci: xpath->path) {
            DILocation* loc = ci->debug_loc();
            ofs << '(' << ci->function()->name() << '+' << ci->get_position_in_function() << ") "
                << loc->filename() << ':' << loc->line() << std::endl;
        }
        ofs << std::endl;
    }

    ofs.close();
}

void ContextGenerator::traverse() {
    auto tos = _stack[_stack.size()-1];
    for (auto ci: tos->function()->caller_list()) {
        _stack.push_back(ci);
        if (_stack.size() < 3) {
            traverse();
        }
        else {

        }
    }
}