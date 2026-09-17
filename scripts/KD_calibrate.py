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


def extract_pts(images):
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
    return all_object_points, all_image_points, image_size

def get_K_D(K, D, obj_pts, img_pts, image_size):
    print(len(obj_pts))
    print(len(img_pts))
    ret, K, D, rvecs, tvecs = cv2.calibrateCamera(
        obj_pts, img_pts, image_size, None, None
    )
    return ret, K, D

def get_common_pts(images1, images2):
    obj_pts = []
    img_pts_1 = []
    img_pts_2 = []
        
    img_size = None

    for fname1, fname2 in zip(images1, images2):
        img_1 = cv2.imread(fname1)
        img_2 = cv2.imread(fname2)

        gray_1 = cv2.cvtColor(img_1, cv2.COLOR_BGR2GRAY)
        gray_2 = cv2.cvtColor(img_2, cv2.COLOR_BGR2GRAY)

        if img_size is None:
            img_size = gray_1.shape[::-1]
            # Detect ChArUco corners independently
        corners_1, ids_1, _, _ = detector.detectBoard(gray_1)
        corners_2, ids_2, _, _ = detector.detectBoard(gray_2)

        # Skip frame if either camera didn't detect any corners
        if ids_1 is None or ids_2 is None:
            continue

        # Flatten ID arrays to match easily
        ids_1 = ids_1.flatten()
        ids_2 = ids_2.flatten()

        # Find the intersection of IDs seen by BOTH cameras in this specific frame
        common_ids = np.intersect1d(ids_1, ids_2)

        if len(common_ids) < 4:  # At least 4 points are needed to compute homography/extrinsics
            continue

        frame_img_pts_1 = []
        frame_img_pts_2 = []
        frame_obj_pts = []

        # Pull the exact physical 3D coordinate layout from the board definition
        board_obj_points = board.getChessboardCorners()

        for common_id in common_ids:
            # Get matching image points from Left camera
            idx_1 = np.where(ids_1 == common_id)[0][0]
            frame_img_pts_1.append(corners_1[idx_1])

            # Get matching image points from Right camera
            idx_2 = np.where(ids_2 == common_id)[0][0]
            frame_img_pts_2.append(corners_2[idx_2])

            # Map to physical 3D coordinate on the board surface
            frame_obj_pts.append(board_obj_points[common_id])

        # Convert to float32 arrays and store
        obj_pts.append(np.array(frame_obj_pts, dtype=np.float32))
        img_pts_1.append(np.array(frame_img_pts_1, dtype=np.float32))
        img_pts_2.append(np.array(frame_img_pts_2, dtype=np.float32))

        return obj_pts, img_pts_1, img_pts_2
    
# Perform standard camera calibration using the extracted points
if __name__ == "__main__":
    # Load calibration images
    image_size = None
    obj_pts_1 = []
    img_pts_1 = []

    obj_pts_2 = []
    img_pts_2 = []

    K_1 = []
    D_1 = []

    K_2 = []
    D_2 = []

    images1 = glob.glob('../sandbox/images/*1_*.png')
    images2 = glob.glob('../sandbox/images/*2_*.png')
        
    images1.sort()
    images2.sort()

    print(images1)
    print(len(images1))
    print(images2)
    print(len(images2))
    
    obj_pts_1, img_pts_1, image_size = extract_pts(images1)
    print("\n")
    obj_pts_2, img_pts_2, _  = extract_pts(images2)
    
    print(type(obj_pts_1), len(obj_pts_1))
    print(type(img_pts_1), len(img_pts_1))

    print(type(obj_pts_2), len(obj_pts_2))
    print(type(img_pts_2), len(img_pts_2))
    print(image_size)
    ret_1, K_1, D_1 = get_K_D(K_1, D_1, obj_pts_1, img_pts_1, image_size)
    ret_2, K_2, D_2 = get_K_D(K_2, D_2, obj_pts_2, img_pts_2, image_size)

    print(f"Reprojection Error 1: {ret_1}")
    print(f"Reprojection Error 2: {ret_2}")

    print(K_1)
    print(K_2)
    
    obj_pts, img_pts_1, img_pts_2 = get_common_pts(images1, images2)

    retval, _, _, _, _, R, T, E, F = cv2.stereoCalibrate(
        obj_pts,
        img_pts_1,
        img_pts_2,
        cameraMatrix1=K_1,          # Passes empty matrix to estimate intrinsically
        distCoeffs1=D_1,
        cameraMatrix2=K_2,
        distCoeffs2=D_2,
        imageSize=image_size,
        flags=cv2.CALIB_FIX_ASPECT_RATIO + cv2.CALIB_ZERO_TANGENT_DIST + cv2.CALIB_FIX_INTRINSIC
    )

    print(f"R: {R}")
    print(f"T: {T}")
    print(f"Dist between Cameras: {np.linalg.norm(T)}")

    np.savez(
        "stereo_data.npz",
        K_1=K_1,
        K_2=K_2,
        D_1=D_1,
        D_2=D_2,
        R=R,
        T=T,
        image_size=image_size
    )
