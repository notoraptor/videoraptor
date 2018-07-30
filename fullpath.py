import os
import sys
if len(sys.argv) == 2:
    path = os.path.abspath(sys.argv[1])
    if sys.platform.startswith('win'):
        path = path.replace('\\', '\\\\')
    print('"%s"' % path)
