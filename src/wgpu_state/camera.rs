pub struct Camera {
    pub eye: cgmath::Point3<f32>,
    pub target: cgmath::Point3<f32>,
    pub up: cgmath::Vector3<f32>,
    pub aspect: f32,
    pub fovy: f32,
    pub z_near: f32,
    pub z_far: f32,
}

#[macro_export]
macro_rules! deg_to_rad {
    ($deg:expr) => {
        $deg * 0.01744444444
    };
}

#[rustfmt::skip]
pub const OPENGL_TO_WGPU_MATRIX: cgmath::Matrix4<f32> = cgmath::Matrix4::new(
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.5,
    0.0, 0.0, 0.0, 1.0,
);

impl Camera {
    pub fn build_view_projection_matrix(&self) -> cgmath::Matrix4<f32>{
        let view = cgmath::Matrix4::look_at_rh(self.eye, self.target, self.up);
        let proj =
            cgmath::perspective(cgmath::Deg(self.fovy), self.aspect, self.z_near, self.z_far);

        return OPENGL_TO_WGPU_MATRIX * proj * view;
    }
}
