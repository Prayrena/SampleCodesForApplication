Right now this program successfully running on my laptop using my Meta Quest3 in SteamVR/OpenXR in Meta compatibility mode. And I think if you connect another brand headset to your laptop, it should work as well.

The OpenXR tutorial online is missing a very important step to create a platform plugin to take the API layers and extensions to work. I thought I was missing any API layers or extensions, costing me all Saturday to see why it failed to create the XRInstance and try to install different versions of the openXR SDK.

There is a screenshot in the zip showing how it looks right now.

The bad news is that the basic environment needs to use Cmake and JasonCpp. I spent a whole Friday afternoon to set it up correctly. However, I could not let it work again and I don't know how to clone my project from D:\helloWorld to D:\P4\SD\Personal

Also, the openXR runs extremely weirdly in that it needs to generate an Openxr_loader to access all the API libraries. So I don't think I could put the related files into my engine. I need some help to move my project to my P4 workspace and link it with my SD engine.
