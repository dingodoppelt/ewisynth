BUNDLE = ewisynth.lv2
PREFIX ?= /usr
INSTALL_DIR ?= $(DESTDIR)$(PREFIX)/lib/lv2
CXXFLAGS ?= -O3 -march=native -flto -std=c++11
CXX ?= g++

$(BUNDLE): manifest.ttl ewisynth.ttl ewisynth.so
	rm -rf $(BUNDLE)
	mkdir $(BUNDLE)
	cp manifest.ttl ewisynth.ttl ewisynth.so $(BUNDLE)

ewisynth.so: ewisynth.cpp
	$(CXX) $(LDFLAGS) $(CXXFLAGS) $(CFLAGS) -fvisibility=hidden -fPIC -Wl,-Bstatic -Wl,-Bdynamic -Wl,--as-needed -shared -pthread `pkg-config --cflags lv2` -lm `pkg-config --libs lv2` ewisynth.cpp -o ewisynth.so

install: $(BUNDLE)
	mkdir -p $(INSTALL_DIR)
	rm -rf $(INSTALL_DIR)/$(BUNDLE)
	cp -R $(BUNDLE) $(INSTALL_DIR)

clean:
	rm ewisynth.so
	rm -rf $(BUNDLE)
