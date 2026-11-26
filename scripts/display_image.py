import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import sys

# 1. Load the image file into a NumPy array
img = mpimg.imread(sys.argv[1])

# 2. Render the array data as an image
plt.imshow(img)

# 3. (Optional) Hide the pixel coordinate axes for a cleaner look
plt.axis('off')

# 4. Open the window or render the output
plt.show()
