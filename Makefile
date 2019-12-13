Sources :=	$(wildcard *.c)
Objects :=	$(patsubst %.c,%.o,$(Sources))
Target :=	testit
CC :=		gcc
CFLAGS :=	-g -Wall -std=gnu11
#LDFLAGS :=	-lm
$(Target):	$(Objects)
.PHONY:		clean depend realclean
clean:
		rm -f $(Objects)
realclean:	clean
		rm -f $(Target)
depend:		
		gcc-makedepend $(CFLAGS) $(Sources)
# DO NOT DELETE
my_alloc.o: my_alloc.c my_alloc.h my_system.h
my_system.o: my_system.c my_system.h
testit.o: testit.c my_alloc.h my_system.h
