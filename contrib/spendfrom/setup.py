from distutils.core import setup
setup(name='NAVspendfrom',
      version='1.0',
      description='Command-line utility for navcoin "coin control"',
      author='Gavin Andresen',
      author_email='gavin@navcoinfoundation.org',
      requires=['jsonrpc'],
      scripts=['spendfrom.py'],
      )
