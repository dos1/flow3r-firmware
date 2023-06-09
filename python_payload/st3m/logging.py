import time

DEBUG = 0
INFO = 1
WARNING = 2
ERROR = 3

#this is so ugly, but works
levels = ["DEBUG","INFO","WARNING","ERROR"]

class Log():
    def __init__(self,name="log",level=INFO):
        self.name = name
        self.level = level
        self.logstring = "{timestamp} {name} ({level}): {msg}"

   
    def debug(self,msg):
        self.message(msg,DEBUG)

    def info(self,msg):
        self.message(msg,INFO)
    
    def warning(self,msg):
        self.message(msg,WARNING)
    
    def error(self,msg):
        self.message(msg,ERROR)
        
    def message(self,msg,level):
        if self.level <= level:
            self._emit(self.logstring.format(
                timestamp = time.ticks_ms()/1000,
                name=self.name, 
                msg = msg, 
                level = levels[level])
        )

    def _emit(self,line):
        print(line)