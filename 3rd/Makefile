all:
	rm -rf ./temp && mkdir temp
	cp -rf ./libuv ./temp/libuv
	cd ./temp/libuv && sh autogen.sh && ./configure && make
	cd ../..
	cp -f ./temp/libuv/.libs/libuv.so* ../lib
	rm -rf ./temp
		
clean:
	rm -rf ./lib/*.so* && rm -rf ./lib/*.a*
