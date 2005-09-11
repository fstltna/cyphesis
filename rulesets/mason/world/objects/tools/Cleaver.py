#This file is distributed under the terms of the GNU General Public license.
#Copyright (C) 1999 Aloril (See the file COPYING for details).

from atlas import *

from world.objects.Thing import Thing
from misc import set_kw

class Cleaver(Thing):
    """This is base class for axes, this one just ordinary axe"""
    def cut_operation(self, op):
        #to_ = self.world.get_object(op[1].id)
        to_ = op[0].id
        if not to_:
            return self.error(op,"To is undefined object")
        return Message(Operation("chop",op[0],Entity(op.from_),to=to_))
