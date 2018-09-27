//
// Created by rebut_p on 28/09/18.
//

#include "userland.h"

struct UserLand {
    u32 base;
    u32 limit;
};

static struct UserLand userLand[4];

void addUserlandEntry(u32 id, u32 base, u32 limit) {
    userLand[id].base = base;
    userLand[id].limit = limit;
}

void enterUserland() {

}
