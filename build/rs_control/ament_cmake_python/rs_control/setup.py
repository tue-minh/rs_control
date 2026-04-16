from setuptools import find_packages
from setuptools import setup

setup(
    name='rs_control',
    version='0.0.0',
    packages=find_packages(
        include=('rs_control', 'rs_control.*')),
)
