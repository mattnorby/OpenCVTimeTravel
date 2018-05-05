# OpenCVTimeTravel
Combine photos from 1997 and 2012 with the same person, using seamless cloning.

This program takes two images as input.
The first (kayak2012.jpg) is a photo of two kayakers in Indiana, USA from 2012.
The second (koln1997.jpg) is a photo of a tourist in Koln, Germany from 1997.
The tourist is one of the kayakers.
The program allows the user to click points in the image to lasso-select a portion of the kayak photo.
That portion is seamlessly cloned into the 1997 photo.

The high-level flow of the program is as follows:
1. Load the kayak image.
2. Resize the kayak to a plausible size.
3. Blur the kayak image, since it is in better focus than the 1997 image.
4. Allow the user to click points on the screen, which will be used as polygon vertices.
5. Create a mask from the polygon selected by the user.
6. Create a mask for the tourist in the foreground.
7. Seamlessly clone the kayak into the 1997 image.
8. Copy the foreground pixels (the tourist) back into the cloned image, to remove artifacts close to the cloned region.
9. Save the image to disk as clone.jpg.
10. Resize the image to fit on screen, and display it.
