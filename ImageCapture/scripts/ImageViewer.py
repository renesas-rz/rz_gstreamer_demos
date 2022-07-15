import numpy as np
from PIL import Image

filename="capImage"

# Read the Binary image file into a Numpy Array
A = np.fromfile(filename, dtype='uint8', sep="")

# The Array is intially a 1D  Array
# Here we reshape the array to VGA resolution with 3 channels of color depth
A = A.reshape([480, 640, 3])

# Now we need to to conver the Numpy array to a Image
# Here we will use PIL Image library
imge_out = Image.fromarray(A.astype('uint8'))
img_as_img = imge_out.convert("RGB")

# PIL image view expects image to be in RGB format. 
# Here we rotate the BGR pixels to RGB
B,G,R = img_as_img.split()
new_image = Image.merge("RGB", (R,G,B))

# Now view the image
new_image.show()
