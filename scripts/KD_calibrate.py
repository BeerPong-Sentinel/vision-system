import cv2
import cv2.aruco as aruco
import glob
import numpy as np
import sys

# Define ChArUco board parameters
SQUARX = 11  # Number of squares in X direction
SQUARY = 7  # Number of squares in Y direction
SQUARE_LENGTH = 0.024  # Side length of a square in meters
MARKER_LENGTH = 0.019  # Side length of an ArUco marker in meters
DICTIONARY_ID = aruco.DICT_6X6_250

# Setup ArUco dictionary and board
dictionary = aruco.getPredefinedDictionary(DICTIONARY_ID)
board = aruco.CharucoBoard((SQUARX, SQUARY), SQUARE_LENGTH, MARKER_LENGTH, dictionary)
detector = aruco.CharucoDetector(board)

# Load calibration images
images1 = glob.glob('../sandbox/images/*1_*.png')
images2 = glob.glob('../sandbox/images/*2_*.png')
print(images1)
print(len(images1))
print(images2)
print(len(images2))

sys.exit()
all_object_points = []  # 3D points in real world space
all_image_points = []   # 2D points in image plane
image_size = None

for fname in images:
    img = cv2.imread(fname)
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    if image_size is None:
        image_size = gray.shape[::-1]

    # Detect ChArUco corners
    charuco_corners, charuco_ids, marker_corners, marker_ids = detector.detectBoard(gray)

    # Check if a board was detected with sufficient corners
    if charuco_corners is not None and charuco_ids is not None and len(charuco_ids) > 4:
        # Match 2D image points to 3D object points using the board layout
        obj_pts, img_pts = board.matchImagePoints(charuco_corners, charuco_ids)
        
        if len(obj_pts) > 4:
            all_object_points.append(obj_pts)
            all_image_points.append(img_pts)

# Perform standard camera calibration using the extracted points
ret, camera_matrix, dist_coeffs, rvecs, tvecs = cv2.calibrateCamera(
    all_object_points, all_image_points, image_size, None, None
)
np.savez(
    "calibration_data_R.npz",
    camera_matrix=camera_matrix,
    dist_coeffs=dist_coeffs,
    image_size=np.array([1920, 1080])
)
print("Reprojection Error:", ret)
print("\nCamera Matrix:\n", camera_matrix)
print("\nDistortion Coefficients:\n", dist_coeffs)
