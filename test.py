# To be run in .local.
from ctypes import *

video_raptor_dll = cdll.videoRaptorBatch
create_output = video_raptor_dll.createOutput
output_to_string = video_raptor_dll.outputToString
delete_output = video_raptor_dll.deleteOutput
video_raptor = video_raptor_dll.videoRaptorBatch
videoRaptorDetails = video_raptor_dll.videoRaptorDetails

class VideoDetails(Structure):
    _fields_ = [
        ("filename", c_char_p),
        ("title", c_char_p),
        ("container_format", c_char_p),
        ("audio_codec", c_char_p),
        ("video_codec", c_char_p),
        ("width", c_int),
        ("height", c_int),
        ("frame_rate_num", c_int),
        ("frame_rate_den", c_int),
        ("sample_rate", c_int),
        ("duration", c_int64),
        ("duration_time_base", c_int64),
        ("size", c_int64),
        ("bit_rate", c_int64)
    ]
    def __str__(self):
        string_view = ''
        for (field_name, _) in self._fields_:
            string_view += '%s;\n' % (getattr(self, field_name))
        return string_view

ARR_LENGTH = 4
arr_t = c_char_p * ARR_LENGTH
video_details_t = VideoDetails * ARR_LENGTH
vd_ptr_t = POINTER(VideoDetails)
vd_arr_t = vd_ptr_t * ARR_LENGTH

details = [VideoDetails()] * ARR_LENGTH
p_v_arr = [pointer(v) for v in details]

create_output.restype = c_void_p
video_raptor.argtypes = [c_int, arr_t, arr_t, c_char_p, c_void_p]
videoRaptorDetails.argtypes = [c_int, arr_t, vd_arr_t, c_void_p]
videoRaptorDetails.restype = c_bool
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
res = videoRaptorDetails(ARR_LENGTH,
                         arr_t(*(x.encode() for x in filenames)),
                         vd_arr_t(*p_v_arr),
                         output)
result = output_to_string(output)
delete_output(output)
print('==========', res, '==========')
print(result.decode())
for detail in details:
    print(detail)
    print(detail.filename)
print('==========')