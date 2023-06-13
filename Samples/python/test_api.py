import hashlib
import numpy as np
import OpenVisus as ov
import os
import shutil
import unittest

DOMAIN = 'klacansky.com'

class TestAPI(unittest.TestCase):
	# TODO(12/10/2022): test local file
	def test_load_dataset(self):
		dataset = ov.load_dataset(f"https://{DOMAIN}/open-scivis-datasets/silicium/silicium.idx")
		self.assertEqual(dataset.max_resolution, 19)
		self.assertEqual(dataset.field_names, ["data"])
		self.assertEqual(dataset.x, (0, 98))
		self.assertEqual(dataset.y, (0, 34))
		self.assertEqual(dataset.z, (0, 34))

		with self.assertRaises(TypeError):
			ov.load_dataset()

		with self.assertRaises(TypeError):
			ov.load_dataset(10)

		with self.assertRaises(FileNotFoundError):
			dataset = ov.load_dataset(f"https://{DOMAIN}/open-scivis-datasets/silicium/silicium.idx_")


	def test_cache_dir(self):
		# test no cache directory is created if it is empty
		dataset = ov.load_dataset(f"https://{DOMAIN}/open-scivis-datasets/silicium/silicium.idx")
		dataset.read()
		self.assertFalse(os.path.isdir("IdxDiskAccess"))

		# test if the cached files are correctly downloaded
		assert not os.path.isdir("test")
		dataset = ov.load_dataset(f"https://{DOMAIN}/open-scivis-datasets/silicium/silicium.idx", cache_dir="test")
		dataset.read()
		self.assertTrue(os.path.exists(f"test/IdxDiskAccess/{DOMAIN}/443/zip/open-scivis-datasets/silicium/silicium.idx"))
		self.assertTrue(os.path.exists(f"test/IdxDiskAccess/{DOMAIN}/443/zip/open-scivis-datasets/silicium/silicium/time_0000/0000.bin"))
		shutil.rmtree("test")

		# TODO(2/19/2023): test if dangling lock file is handled correctly (can happen if process is interrupted)
		#assert not os.path.isdir("test")
		#dataset = ov.load_dataset(f"https://{DOMAIN}/open-scivis-datasets/silicium/silicium.idx", cache_dir="test")
		#dataset.read()
		#os.remove(f"test/{DOMAIN}/443/open-scivis-datasets/silicium/silicium/0000.bin")
		#with open(f"test/{DOMAIN}/443/open-scivis-datasets/silicium/silicium/0000.bin.lock", "w") as f:
		#	pass
		#dataset.read()
		#shutil.rmtree("test")
			
		with self.assertRaises(NotADirectoryError):	
			with open("test.txt", "w") as f:
				f.write("hello\n")
			dataset = ov.load_dataset(f"https://{DOMAIN}/open-scivis-datasets/silicium/silicium.idx", cache_dir="test.txt")


	def test_read(self):
		dataset = ov.load_dataset(f"https://{DOMAIN}/open-scivis-datasets/silicium/silicium.idx")

		data = dataset.read()
		self.assertEqual(data.shape, (dataset.z[1] - dataset.z[0], dataset.y[1] - dataset.y[0], dataset.x[1] - dataset.x[0]))
		self.assertEqual(data.dtype, np.uint8)
		self.assertEqual(hashlib.sha512(data).hexdigest(), "8ef2b9a84eb94693596b57f3f21f5ea75c1c25654011e3aed39a27f5e4259ebbbd2486ff39bb32b551bb44f3fa25123e7128cfd3fc053134f0806e23bb24a819")

		data = dataset.read(x=(10,11))
		self.assertEqual(data.shape, (34,34,1))
		data_slice = dataset.read(x=10)
		self.assertEqual(data_slice.shape, (34, 34))
		self.assertTrue((data == data_slice).all())

		dataset.read(field_name="data")

		with self.assertRaises(ValueError):
			dataset.read(field_name="missing")

		for i, axis in enumerate(["x", "y", "z"]):
			dataset.read(**{axis: getattr(dataset, axis)})

			with self.assertRaises(TypeError):
				dataset.read(**{axis: ("str", 1.5)})

			with self.assertRaises(IndexError):
				dataset.read(**{axis: (-1,10)})
			with self.assertRaises(IndexError):
				dataset.read(**{axis: (0,100)})
			with self.assertRaises(IndexError):
				dataset.read(**{axis: (10,10)})
			with self.assertRaises(IndexError):
				dataset.read(**{axis: (10,-11)})

		self.assertEqual(dataset.read(resolution=0).shape, (1,1,1))
		self.assertTrue((dataset.read(resolution=dataset.max_resolution) == dataset.read()).all())
		with self.assertRaises(ValueError):
			dataset.read(resolution=-1)
		with self.assertRaises(ValueError):
			dataset.read(resolution=dataset.max_resolution + 1)

		#data = dataset.read(max_size_bytes=1024)
		#print(data.shape)

		# TODO: deal with queries that can't find good resolution
		# TODO: can it happend with other types of box queries?
		#data = dataset.read(x=33, resolution=dataset.max_resolution - 3)

if __name__ == '__main__':
	unittest.main(verbosity=2, exit=True)