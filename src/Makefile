TARGET   = render.out
INCLUDES = 
NOMAKEDIR= .git%
OBJDIR = objs
OPTION := -L/usr/local/lib \
		  -lGLEW -lglfw3 -lGLU -lGL -lX11 -lXrandr -lXi -lXxf86vm \
		  -lpthread -lXcursor -lXinerama -ldl


GCC = g++
CFLAGS = -O3
D_OPTION = -MD

CPPS = $(shell find * | grep '.cpp$$')
SRCS = $(filter-out $(IGNORE), $(filter-out $(NOMAKEDIR), $(CPPS)))
DIRS = $(dir $(SRCS))
BINDIRS = $(addprefix $(OBJDIR)/, $(DIRS))

OBJS = $(addprefix $(OBJDIR)/, $(SRCS:.cpp=.o))
DEPS = $(OBJS:.o=.d)
TILS = $(SRCS:.cpp=.cpp~)

ifeq "$(strip $(OBJDIR))" ""
  OBJDIR = .
endif

ifeq "$(strip $(DIRS))" ""
  OBJDIR = .
endif

default:
	@[ -d  $(OBJDIR)   ] || mkdir -p $(OBJDIR)
	@[ -d "$(BINDIRS)" ] || mkdir -p $(BINDIRS)
	@make all --no-print-directory
	./$(TARGET)

all : $(OBJS) $(TARGET)

$(TARGET): $(OBJS) $(LIBS)
	$(GCC) -o $@ $^ $(OPTION)

$(OBJDIR)/%.o: %.cpp
	$(GCC) $(CFLAGS) $(INCLUDES) -o $@ -c $< $(D_OPTION)

clean:
	@rm -rf $(TARGET) $(TILS) $(OBJDIR)

-include $(DEPS)
