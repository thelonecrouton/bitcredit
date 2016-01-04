#!/usr/bin/env python

from setuptools import setup, find_packages
import os

from bitcoin import __version__

here = os.path.abspath(os.path.dirname(__file__))
with open(os.path.join(here, 'README')) as f:
    README = f.read()

requires = []

setup(name='python-bitcreditslib',
      version=__version__,
      description='The Swiss Army Knife of the Bitcoin protocol.',
      long_description=README,
      classifiers=[
          "Programming Language :: Python",
      ],
      url='https://github.com/YarkoL/python-bitcreditslib',
      packages=find_packages(),
      zip_safe=False,
      install_requires=requires,
      test_suite="bitcoin.tests"
     )
