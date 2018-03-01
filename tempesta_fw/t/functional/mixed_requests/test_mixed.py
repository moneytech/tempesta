from helpers import control, tempesta, tf_cfg
from testers import stress

import mixed_test

__author__ = 'Tempesta Technologies, Inc.'
__copyright__ = 'Copyright (C) 2017 Tempesta Technologies, Inc.'
__license__ = 'GPL2'

class GetRequests(mixed_test.MixedRequests):
    """ HEAD, GET requests """
    script = "pipeline"

class RealRequest(mixed_test.MixedRequests):
    """ Real GET request """
    script = "get_real"

class RealRequest2(mixed_test.MixedRequests):
    """ Real GET request 2"""
    script = "get_real_2"

class RealRequestPipeline(mixed_test.MixedRequests):
    """ Real pipelined GET request """
    script = "get_real_pipelined"

class GetPostRequests(mixed_test.MixedRequests):
    """ HEAD, GET requests """
    script = "get_post"

class HeadGetRequests(mixed_test.MixedRequests):
    """ HEAD, GET requests """
    script = "head_get"

class EmptyPostRequests(mixed_test.MixedRequests):
    """ POST requests """
    script = "post_empty"

class SmallPostRequests(mixed_test.MixedRequests):
    """ POST requests """
    script = "post_small"

class BigPostRequests(mixed_test.MixedRequests):
    """ POST requests """
    script = "post_big"

class RarelyUsedRequests(mixed_test.MixedRequests):
    """ Rarely used requests """
    script = "mixed"