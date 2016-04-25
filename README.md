# Cinder Samples
Personal samples and experiments made with Cinder broken down into easily consumable chunks

All samples have been created in XCode on OSX, so there's no guarantee that they will work in windows, iOS, android, or Linux, unless noted. I plan on making all samples compilable on Windows soon.

### Geometry and Lights
This sample was made to explore how to build out a style for a game that was quickly abandoned.

It showcases:
* Wireframe rendering in a Geometry shader
* Point light rendering in Fragment shader
* Vertex shader vertex manipulation


### Image Transitions
This is a work in progress for to play with different image transitions that can be used in any sort of image gallery. Very incomplete at the moment.

### Instancing
This sample cobbles together some techniques used in the IBM Crunchers Table-Top arcade game to render and control multiple sprite instances. The arrows sample within uses a simple sprite sheet to render different arrows within a sprite sheet.

### SpiderWeb
This is an evolution of some spider web generation studies that I initially did in processing and javascript a few years ago. This brings my initial studies into Cinder and uses transform feedback to efficiently manipulate the physics of each individual point of the spider web.

### TextParticles
This is an update to an older version of exploding text particles used in a project that used an FBO ping-ponging technique in Cinder 0.8.6. This update works in Cinder 0.9.0+ using transform feedback.

**To use:**
* Start typing a word
* Hit the "Enter" key to explode
* Play with the gui on the left side to play with different parameters and hit enter to see the updated particle simulation
