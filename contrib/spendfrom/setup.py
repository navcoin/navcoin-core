from distutils.core import setup
setup(name='DVTspendfrom',
      version='1.0',
      description='Command-line utility for devault "coin control"',
      author='Gavin Andresen',
      author_email='gavin@devaultfoundation.org',
      requires=['jsonrpc'],
      scripts=['spendfrom.py'],
      )
