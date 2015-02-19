#  Gentle Valley Stream PiCam h264 RTP Sender controllable by ZOCP
#  Copyright (C) 2015  Arnaud Loonstra
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#
# Required:
# * Gstreamer1.0
# * GstPyCamSrc
# * PyZOCP

import gi
gi.require_version('Gst', '1.0')
from gi.repository import GObject, Gst
print(Gst.version())
from zocp import ZOCP
import zmq
import socket

def bus_call(bus, msg, *args):
    #print("BUSCALL", msg, msg.type, *args)
    if msg.type == Gst.MessageType.EOS:
        print("End-of-stream")
        loop.quit()
        return
    elif msg.type == Gst.MessageType.ERROR:
        print("GST ERROR", msg.parse_error())
        loop.quit()
        return
    return True

def zocp_handle(*args, **kwargs):
    z.run_once()
    return True

def zocp_on_modified(peer, name, data, *args, **kwargs):
    print(peer, name, data, args, kwargs)
    if data.get('quit'):
        loop.quit()
        return
    for key, value in data.items():
        if 'value' in value.keys():
            try:
                videosrc.set_property(key, value['value'])
            except Exception as e:
                print("Failed setting property {0} to {1} : {3}".format(key, value, e))
            else:
                print("set {0} to {1}".format(key, value['value']))

if __name__ == "__main__":
    GObject.threads_init()
    # initialization
    loop = GObject.MainLoop()
    Gst.init(None)
    # create elements
    pipeline = Gst.Pipeline()
    # watch for messages on the pipeline's bus (note that this will only
    # work like this when a GLib main loop is running)
    bus = pipeline.get_bus()
    bus.add_watch(0, bus_call, loop) # 0 == GLib.PRIORITY_DEFAULT 

    # create elements
    videosrc = Gst.ElementFactory.make('rpicamsrc', "videosrc0")
    #videosrc = Gst.ElementFactory.make('v4l2src', 'videosrc0')
    h264parse = Gst.ElementFactory.make("h264parse", 'h264parse0')
    rtppay = Gst.ElementFactory.make("rtph264pay", 'rtph264pay0')
    sink = Gst.ElementFactory.make("udpsink", 'udpsink0')

    # change video source caps
    caps = Gst.Caps.from_string("video/x-raw, width=320, height=240")
    #caps = Gst.Caps.from_string("application/x-rtp,encoding-name=H264,payload=96")

    #videosrc.set_property("num-buffers", 400)
    videosrc.set_property("bitrate", 1000000)
    h264parse.set_property("config-interval", 1)
    sink.set_property("host", "192.168.18.117")
    sink.set_property("port", 5000)

    # add elements
    pipeline.add(videosrc)
    pipeline.add(h264parse)
    pipeline.add(rtppay)
    pipeline.add(sink)

    # link elements 
    videosrc.link(h264parse)
    h264parse.link(rtppay)
    rtppay.link(sink)
    #decoder.link(queue)
    #queue.link(glimagesink)
    

    # zocp 
    z = ZOCP()
    z.set_name("RpiCamStreamer@{0}".format(socket.gethostname()))
    z.register_bool("quit", False, access='rw')
    z.register_bool("do-timestamp", True, access='rw')
    z.register_int("bitrate", 1000000, access='rw', min=1, max=25000000)
    z.register_int("keyframe-interval", 25, access='rw')
    z.register_bool("preview", True, access='rw')
    z.register_bool("preview-encoded", True, access='rw')
    z.register_int("preview-opacity", 255, access='rw', min=0, max=255)
    z.register_bool("fullscreen", True, access='rw')
    z.register_int("sharpness", 0, access='rw', min=-100, max=100)
    z.register_int("contrast", 0, access='rw', min=-100, max=100)
    z.register_int("brightness", 50, access='rw', min=0, max=100)
    z.register_int("saturation", 0, access='rw', min=-100, max=100)
    z.register_int("iso", 0, access='rw', min=0, max=3200)
    z.register_bool("video-stabilisation", False, access='rw')
    z.register_int("exposure-compensation", 0, access='rw', min=-10, max=10)
    z.register_string("exposure-mode", "auto", access='rw')
    z.register_string("metering-mode", "average", access='rw')
    z.register_string("awb-mode", "auto", access='rw')
    z.register_string("image-effect", "none", access='rw')
    z.register_int("rotation", 0, access='rw', min=0, max=270)
    z.register_bool("hflip", False, access='rw')
    z.register_bool("vflip", False, access='rw')
    z.register_float("roi-x", 0., access='rw', min=0., max=1.0)
    z.register_float("roi-y", 0., access='rw', min=0., max=1.0)
    z.register_float("roi-w", 1., access='rw', min=0., max=1.0)
    z.register_float("roi-h", 1., access='rw', min=0., max=1.0)
    # set the on_modified method to our own method
    z.on_modified = zocp_on_modified
    z.start()
    # listen to the zocp inbox socket
    GObject.io_add_watch(
        z.inbox.getsockopt(zmq.FD), 
        GObject.PRIORITY_DEFAULT, 
        GObject.IO_IN, zocp_handle
    )
    # update the fisplay every 1/60s
    #GObject.idle_add(glib_idle)

    # run
    pipeline.set_state(Gst.State.PLAYING)
    try:
        loop.run()
    except Exception as e:
        print(e)
    finally:
        z.stop()

    # cleanup
    pipeline.set_state(Gst.State.NULL)
