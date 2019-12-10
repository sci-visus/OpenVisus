

class Dog(object):
    name = ''
    moves = []

    def __init__(self, name):
        self.name = name

    def moves_setup(self,x):
        self.moves.append('walk')
        self.moves.append('run')
        self.moves.append(x)
    def get_moves(self):
        return self.moves

class Superdog(Dog):

    #Let's try to append new fly ability to our Superdog
    def moves_setup(self):
        #Set default moves by calling method of parent class
        super().moves_setup("hello world")
        self.moves.append('fly')
dog = Superdog('Freddy')
print (dog.name)
dog.moves_setup()
print (dog.get_moves()) 


