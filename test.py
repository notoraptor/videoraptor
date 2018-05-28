# To be run in .local.
from ctypes import *

video_raptor_dll = cdll.videoRaptorBatch
create_output = video_raptor_dll.createOutput
output_to_string = video_raptor_dll.outputToString
delete_output = video_raptor_dll.deleteOutput
video_raptor = video_raptor_dll.videoRaptorBatch

ARR_LENGTH = 4
arr_t = c_char_p * ARR_LENGTH

create_output.restype = c_void_p
video_raptor.argtypes = [c_int, arr_t, arr_t, c_char_p, c_void_p]
output_to_string.argtypes = [c_void_p]
output_to_string.restype = c_char_p
delete_output.argtypes = [c_void_p]

filenames = ['I:\\donnees\\autres\\p\\BBECA9~1.MP4',
             'I:\\donnees\\autres\\p\\BB368C~1.MP4',
             'I:\\donnees\\autres\\p\\BB8444~1.MP4',
             'I:\\donnees\\autres\\p\\BB07D8~1.MP4']
thumbnames = ['a', 'b', None, None]
thumbFolder = '.local'

output = create_output()
video_raptor(ARR_LENGTH,
             arr_t(*(x.encode() for x in filenames)),
             arr_t(*((x.encode() if x is not None else x) for x in thumbnames)),
             thumbFolder.encode(),
             output)
result = output_to_string(output)
delete_output(output)
print('==========')
print(result.decode())
print('==========')