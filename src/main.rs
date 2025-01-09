use std::error::Error;

mod vk;

use winit::{
    application::ApplicationHandler, dpi::PhysicalSize, event::WindowEvent, event_loop::EventLoop,
    raw_window_handle::HasWindowHandle, window::Window,
};

#[derive(Default)]
pub struct Application {
    window: Option<Window>,
    vk: vk::Vk,
    egui_ctx: egui::Context,
}

impl ApplicationHandler for Application {
    fn resumed(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
        let window_size = PhysicalSize::new(1280, 720);
        self.window = Some(
            event_loop
                .create_window(
                    Window::default_attributes()
                        // .with_max_inner_size(window_size)
                        .with_min_inner_size(window_size),
                )
                .unwrap(),
        );

        self.vk = vk::Vk::init(
            self.window
                .as_ref()
                .unwrap()
                .window_handle()
                .unwrap()
                .as_raw(),
        );
    }

    fn window_event(
        &mut self,
        event_loop: &winit::event_loop::ActiveEventLoop,
        window_id: winit::window::WindowId,
        event: winit::event::WindowEvent,
    ) {
        match event {
            WindowEvent::CursorMoved {
                device_id,
                position,
            } => {}
            WindowEvent::RedrawRequested => {
                let input = egui::RawInput::default();
                let full_output = self.egui_ctx.run(input, |ctx| {
                    egui::CentralPanel::default().show(&ctx, |ui| {
                        ui.label("Hello World!!!");
                    });
                });

                let clipped_prims = self
                    .egui_ctx
                    .tessellate(full_output.shapes.clone(), full_output.pixels_per_point);

                self.vk.render(&clipped_prims, full_output);
                self.window.as_ref().unwrap().request_redraw();
            }
            WindowEvent::Resized(new_size) => {
                println!("new size: {:?}", new_size);
            }
            WindowEvent::CloseRequested => {
                let input = egui::RawInput::default();
                let full_output = self.egui_ctx.run(input, |ctx| {
                    egui::CentralPanel::default().show(&ctx, |ui| {
                        ui.label("Hello World!!!");
                    });
                });

                let clipped_prims = self
                    .egui_ctx
                    .tessellate(full_output.shapes.clone(), full_output.pixels_per_point);

                // for clipped_prim in clipped_prims {
                    // match clipped_prim.primitive {
                    // egui::epaint::Primitive::Mesh(mesh) => {
                    //     for vert in mesh.vertices {
                    //         println!(
                    //             "pos: {:?} color: {} {} {} {} uv: {:?}",
                    //             vert.pos,
                    //             vert.color.r(),
                    //             vert.color.g(),
                    //             vert.color.b(),
                    //             vert.color.a(),
                    //             vert.uv
                    //         );
                    //     }
                    //     println!("texture id: {:?}", mesh.texture_id);
                    // }

                    // egui::epaint::Primitive::Callback(cb) => {
                    //     println!("cb {:?}", cb);
                    // }
                    // }
                // }

                // println!(
                //     "tex delta set len: {} free len: {}",
                //     full_output.textures_delta.set.len(),
                //     full_output.textures_delta.free.len()
                // );

                event_loop.exit();
            }
            _ => (),
        }
    }

    fn new_events(
        &mut self,
        event_loop: &winit::event_loop::ActiveEventLoop,
        cause: winit::event::StartCause,
    ) {
        let _ = (event_loop, cause);
    }

    fn user_event(&mut self, event_loop: &winit::event_loop::ActiveEventLoop, event: ()) {
        let _ = (event_loop, event);
    }

    fn device_event(
        &mut self,
        event_loop: &winit::event_loop::ActiveEventLoop,
        device_id: winit::event::DeviceId,
        event: winit::event::DeviceEvent,
    ) {
        let _ = (event_loop, device_id, event);
    }

    fn about_to_wait(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
        let _ = event_loop;
    }

    fn suspended(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
        let _ = event_loop;
    }

    fn exiting(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
        let _ = event_loop;
        self.vk.shutdown();
    }

    fn memory_warning(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
        let _ = event_loop;
    }
}

fn main() -> Result<(), Box<dyn Error>> {
    println!("Hello");
    let event_loop = EventLoop::new()?;
    event_loop.set_control_flow(winit::event_loop::ControlFlow::Wait);
    let mut app = Application::default();
    event_loop.run_app(&mut app)?;

    // println!("Press any key to continue...");
    // let mut buf = String::new();
    // std::io::stdin()
    //     .read_line(&mut buf)
    //     .expect("failed to continue");

    println!("Bye");
    Ok(())
}
