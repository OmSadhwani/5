CC = gcc
CFLAGS = -I. -L.
SRC_MSOCKET = msocket.c
OBJ_MSOCKET = $(SRC_MSOCKET:.c=.o)
LIB_MSOCKET = libmsocket.a

SRC_INITMSOCKET = initsocket.c
OBJ_INITMSOCKET = $(SRC_INITMSOCKET:.c=.o)
TARGET_INITMSOCKET = initmsocket

SRC_USER1 = tempuser1.c
OBJ_USER1 = $(SRC_USER1:.c=.o)
TARGET_USER1 = tempuser1

SRC_USER2 = tempuser2.c
OBJ_USER2 = $(SRC_USER2:.c=.o)
TARGET_USER2 = tempuser2

LIBS = -lmsocket

all: $(LIB_MSOCKET) $(TARGET_INITMSOCKET) $(TARGET_USER1) $(TARGET_USER2)

$(LIB_MSOCKET): $(OBJ_MSOCKET) msocket.h
		ar rcs $@ $^

$(TARGET_INITMSOCKET): $(OBJ_INITMSOCKET) $(LIB_MSOCKET)
		$(CC) $(CFLAGS) -o $@ $^ -lpthread

$(TARGET_USER1): $(OBJ_USER1) $(LIB_MSOCKET)
		$(CC) $(CFLAGS) -o $@ $^ $(LIBS) -lpthread

$(TARGET_USER2): $(OBJ_USER2) $(LIB_MSOCKET)
		$(CC) $(CFLAGS) -o $@ $^ $(LIBS) -lpthread

%.o: %.c
		$(CC) $(CFLAGS) -c -o $@ $< -lpthread

clean:
		rm -f $(OBJ_MSOCKET) $(LIB_MSOCKET) $(OBJ_INITMSOCKET) $(TARGET_INITMSOCKET) $(OBJ_USER1) $(TARGET_USER1) $(OBJ_USER2) $(TARGET_USER2) *r.txt

run_initmsocket: $(TARGET_INITMSOCKET)
		./$(TARGET_INITMSOCKET)

run_user1: $(TARGET_USER1)
		./$(TARGET_USER1)

run_user2: $(TARGET_USER2)
		./$(TARGET_USER2)
