#/usr/bin/env python3
#
# Copyright (c) 2014 Stichting z25.org
# Copyright (c) 2014 Arnaud Loonstra <arnaud@sphaero.org>
# RPI Stream Receiver
#
# RPI Stream Receiver is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Use the following pipeline for a sender:
# gst-launch-1.0 -v videotestsrc ! video/x-raw,frame-rate=10/1 ! x264enc speed-preset=1 tune=zero-latency byte-stream=true intra-refresh=true option-string="bframes=0:force-cfr:no-mbtree:sync-lookahead=0:sliced-threads:rc-lookahead=0" ! video/x-h264,profile=high ! rtph264pay config-interval=1 ! udpsink host=127.0.0.1 port=5000

import gi
gi.require_version('Gst', '1.0')
from gi.repository import GObject, Gst
print(Gst.version())

from array import array

import os
DESKTOP=True
if os.uname()[4][:3] == 'arm':
    from pogles.egl import *
    from pogles.gles2 import *
    DESKTOP=False
else:
    from OpenGL.GL import *
    from OpenGL.GLUT import *
    from OpenGL import arrays
    from OpenGL import GLX
    
from zocp import ZOCP
import zmq
import socket

vertex_shader_source = """
attribute vec4 a_position;
attribute vec2 a_texCoord;
varying vec2 v_texCoord;
void main()
{
   gl_Position = a_position;
   v_texCoord = a_texCoord;
}
"""
   
frament_shader_source = """
#ifdef GL_ES
precision mediump float;
#endif
varying vec2 v_texCoord;
uniform sampler2D tex;
void main()
{
    gl_FragColor = texture2D( tex, v_texCoord );
}
"""


# Create a shader object, load the shader source, and compile the shader.
def load_shader(shader_type, shader_source):
    # Create the shader object.
    shader = glCreateShader(shader_type)
    if shader == 0:
        return 0

    # Load the shader source.
    glShaderSource(shader, shader_source)
   
    # Compile the shader.
    glCompileShader(shader)

    # Check the compile status.
    compiled = glGetShaderiv(shader, GL_COMPILE_STATUS)
    if not compiled:
        glDeleteShader(shader)
        raise GLException("Error compiling shader:\n%s" % glGetShaderInfoLog(shader))

    return shader

def reshape(width,height):
    #glViewport(0, 0, width, height)
    reshape_callback(None, width, height)

def keyboard( key, x, y ):
    if key == '\033':
        sys.exit( )

def initGL():
    global program
    # Load the vertex/fragment shaders.
    vertex_shader = load_shader(GL_VERTEX_SHADER, vertex_shader_source)
    fragment_shader = load_shader(GL_FRAGMENT_SHADER, frament_shader_source);

    # Create the program.
    program = glCreateProgram()
    if program == 0:
        return 0

    glAttachShader(program, vertex_shader)
    glAttachShader(program, fragment_shader)

    # Bind vPosition to attribute 0.
    glBindAttribLocation(program, 0, "vPosition".encode('ascii'))

    # Link the program.
    glLinkProgram(program)

    # Check the link status.
    linked = glGetProgramiv(program, GL_LINK_STATUS)
    if not linked:
        glDeleteProgram(program)

        raise GLException(
                "Error linking program:\n%s" % glGetProgramInfoLog(program))

    return program

def bus_call(bus, msg, *args):
    if msg.type == Gst.MessageType.EOS:
        print("End-of-stream")
        loop.quit()
        return
    elif msg.type == Gst.MessageType.ERROR:
        print(msg.parse_error())
        loop.quit()
        return
    return True
        

def reshape_callback(glsink, width, height):
    print("reshape", width, height)
    glViewport(0, 0, width, height);
    return True

def glib_idle(*args, **kwargs):
    #GObject.timeout_add(1/60.0, idle_display)
    print("glib idle:", args, kwargs)
    #glutMainLoopEvent()
    #draw_callback(None, None, None, None)
    return True

g_tex = None
# Draw a triangle using the shaders.
def draw_callback(glsink, texture, width, height):
    global program
    if texture:
        g_tex = texture

    if not program:
        if not texture:
            return True
        initGL()
    print("draw", program)
    tl = z.capability['top_left']['value']
    tr = z.capability['top_right']['value']
    br = z.capability['bottom_right']['value']
    bl = z.capability['bottom_left']['value']
    vVertices = [tl[0],  tl[1],  0.0, # pos 0 
                            0.0, 0.0,            # texcoord 0
                            bl[0],  bl[1],  0.0, # pos 1
                            0.0, 1.0,            # texcoord 1
                            br[0],  br[1], 0.0,  # pos 2
                            1.0, 1.0,            # texcoord 2
                            tr[0],  tr[1],  0.0, # pos 3
                            1.0, 0.0, ]         # texcoord 3
    indices = [0, 1, 2, 0, 2, 3]
      
    # Clear the color buffer.
    glClear(GL_COLOR_BUFFER_BIT)

    # Use the program object.
    glUseProgram(program)

    # Request a buffer slot from GPU
    #buffer = glGenBuffers(1)
    
    # Make this buffer the default one
    #glBindBuffer(gl.GL_ARRAY_BUFFER, buffer)
    
    # Upload data
    #glBufferData(gl.GL_ARRAY_BUFFER, data.nbytes, data, gl.GL_DYNAMIC_DRAW)

    # Load the vertex data.
    positionLoc = glGetAttribLocation( program, "a_position".encode('ascii') )
    glVertexAttribPointer(positionLoc, 3, GL_FLOAT, False, 5*4, vVertices)

    # Load the texture coordinate
    texCoordLoc = glGetAttribLocation( program, "a_texCoord".encode('ascii'))
    glVertexAttribPointer ( texCoordLoc, 2, GL_FLOAT, False, 5*4, vVertices[3:])

    glEnableVertexAttribArray(positionLoc)
    glEnableVertexAttribArray(texCoordLoc)
    
    glActiveTexture(GL_TEXTURE0)
    glBindTexture (GL_TEXTURE_2D, g_tex);
    # Set the texture sampler to texture unit 0
    tex = glGetUniformLocation(program, "tex".encode('ascii'))
    glUniform1i ( tex, 0 )
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices)

    return True


def zocp_handle(*args, **kwargs):
    z.run_once()
    if z.capability['quit']['value']:
        loop.quit()
    return True


if __name__ == "__main__":
    GObject.threads_init()
    ret = Gst.StateChangeReturn
    #Gst.Element pipeline 
    #Gst.Element videosrc
    #Gst.Element glimagesink
    # initialization
    loop = GObject.MainLoop()
    #bus = Gst.Bus
    Gst.init(None)
    # create elements
    pipeline = Gst.Pipeline()
    # watch for messages on the pipeline's bus (note that this will only
    # work like this when a GLib main loop is running)
    bus = pipeline.get_bus()
    bus.add_watch(0, bus_call, loop) # 0 == GLib.PRIORITY_DEFAULT 

    # create elements
    #videosrc = Gst.ElementFactory.make('videotestsrc', "videotestsrc0")
    videosrc = Gst.ElementFactory.make('udpsrc', 'videosrc0')
    depay = Gst.ElementFactory.make('rtph264depay', 'depay0')
    decoder = Gst.ElementFactory.make('avdec_h264', 'decoder0')
    queue = Gst.ElementFactory.make('queue', 'queue0')
    glimagesink = Gst.ElementFactory.make('glimagesink', "glimagesink0")

    # change video source caps
    #caps = Gst.Caps.from_string("video/x-raw, format=RGB, width=320, height=240, framerate=25/1")
    caps = Gst.Caps.from_string("application/x-rtp,encoding-name=H264,payload=96")

    # GL program
    program = None
    # Get context:
    #ctx = GLX.glXGetCurrentContext()
    #gst_gl_context = gst_gl_context_new_wrapped (display, (guintptr) sdl_gl_context,
    #  gst_gl_platform_from_string (platform), GST_GL_API_OPENGL);
    # configure elements
    #videosrc.set_property("num-buffers", 400)
    videosrc.set_property("port", 5000)
    #glimagesink.set_property("other-context", ctx)
    glimagesink.connect("client-draw", draw_callback)
    glimagesink.connect("client-reshape", reshape_callback)

    # add elements
    pipeline.add(videosrc)
    pipeline.add(depay)
    pipeline.add(decoder)
    pipeline.add(queue)
    pipeline.add(glimagesink)

    # link elements 
    #videosrc.link_filtered(glimagesink, caps)
    videosrc.link_filtered(depay, caps)
    depay.link(decoder)
    decoder.link(queue)
    queue.link(glimagesink)
    

    # zocp 
    z = ZOCP()
    z.set_name("3DStreamer@{0}".format(socket.gethostname()))
    z.register_bool("quit", False, access='rw')
    z.register_int("port", 5000, access='r')    
    z.register_vec2f("top_left", (-1.0, 1.0), access='rw', step=[0.01, 0.01])
    z.register_vec2f('top_right', (1.0, 1.0), access='rw', step=[0.01, 0.01])
    z.register_vec2f('bottom_right', (1.0, -1.0), access='rw', step=[0.01, 0.01])
    z.register_vec2f('bottom_left', (-1.0, -1.0), access='rw', step=[0.01, 0.01])
    z.start()
    # listen to the zocp inbox socket
    GObject.io_add_watch(
        z.inbox.getsockopt(zmq.FD), 
        GObject.PRIORITY_DEFAULT, 
        GObject.IO_IN, zocp_handle
    )
    # update the fisplay every 1/60s
    GObject.idle_add(glib_idle)

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
