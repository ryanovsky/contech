CLEAN_DIRS=\
common/eventLib \
common/runtime \
common/taskLib \
backend/CommTracker \
backend/Statistics \
backend/SCacheOrder \
backend/Comm2 \
backend/CacheCoherence \
backend/CacheCoherence/tm \
backend/TaskGraphFrontEnd \
backend/Heltech \
backend/Harmony \
backend/RemSync \
middle \

GRAPHVIZ_TOOLS = \
backend/DynamicCFG \
backend/TaskGraphVisualizer \


MAKE_DIRS = $(CLEAN_DIRS)


all: multimake

multimake:
	for d in $(MAKE_DIRS);	\
	do			\
		echo ""; echo -e "\033[94m$$d \033[0m"; \
		make -C $$d;	\
		if [[ $$? -ne 0 ]]; then \
        		echo -e "\033[91m Compilation failed! \033[0m"; \
        		exit 1; \
		fi; \
	done;

viz_backends:
	for d in $(GRAPHVIZ_TOOLS);  \
	do                      \
                echo ""; echo -e "\033[94m$$d \033[0m"; \
                make -C $$d;    \
                if [[ $$? -ne 0 ]]; then \
                        echo -e "\033[91m Compilation failed! \033[0m"; \
                        exit 1; \
                fi; \
        done;

clean:
	for d in $(CLEAN_DIRS);	\
	do			\
		make clean -C $$d;	\
	done;			\


