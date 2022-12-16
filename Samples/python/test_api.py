import hashlib
import numpy as np
import OpenVisus as ov
import unittest


class TestAPI(unittest.TestCase):
	# TODO(12/10/2022): test local file
	def test_load_dataset(self):
		dataset = ov.load_dataset("https://klacansky.com/open-scivis-datasets/silicium/silicium.idx")
		self.assertEqual(dataset.shape, (34,34,98))
		self.assertEqual(dataset.max_resolution, 19)
		self.assertEqual(dataset.field_names, ["data"])

		with self.assertRaises(TypeError):
			ov.load_dataset()

		# TODO(12/10/2022): better error message
		with self.assertRaises(TypeError):
			ov.load_dataset(10)

		with self.assertRaises(FileNotFoundError):
			dataset = ov.load_dataset("https://klacansky.com/open-scivis-datasets/silicium/silicium.idx_")

		dataset = ov.load_dataset("https://klacansky.com/open-scivis-datasets/silicium/silicium.idx", cache_dir="test")

		with self.assertRaises(NotADirectoryError):	
			with open("test.txt", "w") as f:
				f.write("hello\n")
			dataset = ov.load_dataset("https://klacansky.com/open-scivis-datasets/silicium/silicium.idx", cache_dir="test.txt")

	def test_read(self):
		dataset = ov.load_dataset("https://klacansky.com/open-scivis-datasets/silicium/silicium.idx")

		data = dataset.read()
		self.assertEqual(data.shape, dataset.shape)
		self.assertEqual(data.dtype, np.uint8)
		self.assertEqual(hashlib.sha512(data).hexdigest(), "8ef2b9a84eb94693596b57f3f21f5ea75c1c25654011e3aed39a27f5e4259ebbbd2486ff39bb32b551bb44f3fa25123e7128cfd3fc053134f0806e23bb24a819")

		self.assertEqual(dataset.read(x=[10,11]).shape, (34,34,1))

		dataset.read(field_name="data")

		with self.assertRaises(ValueError):
			dataset.read(field_name="missing")

		for i, axis in enumerate(['x', 'y', 'z']):
			dataset.read(**{axis: [dataset.shape[-i - 1] - 1, dataset.shape[-i - 1]]})

			with self.assertRaises(TypeError):
				dataset.read(**{axis: 10})
			with self.assertRaises(TypeError):
				dataset.read(**{axis: ['str', 1.5]})

			with self.assertRaises(IndexError):
				dataset.read(**{axis: [-1,10]})
			with self.assertRaises(IndexError):
				dataset.read(**{axis: [0,100]})
			with self.assertRaises(IndexError):
				dataset.read(**{axis: [10,10]})
			with self.assertRaises(IndexError):
				dataset.read(**{axis: [10,-11]})

		self.assertEqual(dataset.read(max_resolution=0).shape, (1,1,1))
		self.assertTrue((dataset.read(max_resolution=dataset.max_resolution) == data).all())
		with self.assertRaises(ValueError):
			dataset.read(max_resolution=-1)
		with self.assertRaises(ValueError):
			dataset.read(max_resolution=dataset.max_resolution + 1)