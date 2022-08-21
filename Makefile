all:
	python3 -m pip install -e .

clean:
	rm -rf build *.egg-info
	rm -f nulltracer/*.so
