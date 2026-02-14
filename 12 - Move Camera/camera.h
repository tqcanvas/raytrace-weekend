#ifndef CAMERA_H
#define CAMERA_H

#include "hittable.h"
#include "material.h"

class camera {
public:
    double aspect_ratio      = 1.0; // Ratio of image width over height
    int    image_width       = 100; // Rendered image width in pixel count
    int    samples_per_pixel = 10;  // Count of random samples for each pixel
    int    max_depth         = 10;  // Maximum number of ray bounces into scene until no color return

    double vert_fov = 90; // Vertical view angle (field of view)
    point3 lookfrom = point3(0, 0, 0); // Point the camera is looking from (center)
    point3 lookat = point3(0, 0, -1); // Point the camera is looking at
    vec3 vup = vec3(0, 1, 0); // Camera-relative vector for up direction

    void render(const hittable& world) {
        initialize();

        std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";
        for (int j = 0; j < image_height; j++) {
            std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
            for (int i = 0; i < image_width; i++) {
                // Aggregate pixel samples from random selection within block range and sum to pixel_color
                color pixel_color(0, 0, 0);
                for (int sample = 0; sample < samples_per_pixel; sample++) {
                    ray r = get_ray(i, j);
                    pixel_color += ray_color(r, max_depth, world);
                }

                write_color(std::cout, pixel_samples_scale * pixel_color);
            }
        }

        std::clog << "\rDone.                 \n";
    }

private:
    int    image_height;        // Rendered image height
    double pixel_samples_scale; // Color scale factor for a sum of pixel samples
    point3 center;              // Camera center
    point3 pixel00_loc;         // Location of pixel 0, 0
    vec3   pixel_delta_u;       // Offset to pixel to the right
    vec3   pixel_delta_v;       // Offset to pixel below
    vec3 u, v, w;               // Camera frame basis vectors - u is right, v is up, and w is back

    void initialize() {
        // Set image dimensions
        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        pixel_samples_scale = 1.0 / samples_per_pixel;

        center = lookfrom;

        // Viewport dimensions
        auto focal_length = (lookfrom - lookat).length();
        auto theta = degrees_to_radians(vert_fov);
        auto h = std::tan(theta/2);
        auto viewport_height = 2 * h * focal_length;
        auto viewport_width = viewport_height * (double(image_width)/image_height);

        // Calculate camera basis vectors for the camera coordinate frame
        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        // Calculate vectors across horizontal and down vertical viewport edges;
        auto viewport_u = viewport_width * u;
        auto viewport_v = viewport_height * -v;

        // Calc horizontal and vertical delta vectors from pixel to pixel
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // Calc the location of upper left pixel
        auto viewport_upper_left = center - (focal_length * w) - viewport_u/2 - viewport_v/2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
    }

    ray get_ray(int i, int j) const {
        // Construct a camera ray originating from origin and directed at randomly sampled point
        // around the pixel location i, j

        auto offset = sample_square();
        auto pixel_sample = pixel00_loc
                          + ((i + offset.x()) * pixel_delta_u)
                          + ((j + offset.y()) * pixel_delta_v);
        auto ray_origin = center;
        auto ray_direction = pixel_sample - ray_origin;
        return ray(ray_origin, ray_direction);
    }


    vec3 sample_square() const {
        // Returns vector to a random point in the [-0.5, -0.5] - [0.5, 0.5] unit square.
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }

    color ray_color(const ray& r, int depth, const hittable& world) const {
        // When we exceed ray bounce limit, no more light is gathered (return black)
        if (depth <= 0) return color(0, 0, 0);

        hit_record rec;

        // Use a small left bound to avoid shadow acne from surface bounces due to floating point error
        if (world.hit(r, interval(0.001, infinity), rec)) {
            ray scattered;
            color attenuation;
            if (rec.mat->scatter(r, rec, attenuation, scattered))
                return attenuation * ray_color(scattered, depth-1, world);
            return color(0, 0, 0); // Material absorbed the light
        }

        // lerp blue to white gradient
        vec3 unit_direction = unit_vector(r.direction());
        // Normalize y direction of ray from [-1.0, 1.0] to [0, 1.0]
        auto a = 0.5*(unit_direction.y() + 1.0);
        return (1.0 - a) * color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
    }
};

#endif //CAMERA_H
